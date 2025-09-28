#include "include/pipelinedBurst.h"
#include "utils/klog.h"

#include <cstring>
#include <hiredis/hiredis.h>
#include <poll.h>
#include <stdexcept>
#include <thread>

namespace iagent {
static redisContext *connectWithTimeout(const Endpoint &ep) {
  timeval tv{};
  tv.tv_sec = ep.connect_timeout_ms / 1000;
  tv.tv_usec = (ep.connect_timeout_ms % 1000) * 1000;
  redisContext *c = redisConnectWithTimeout(ep.host.c_str(), ep.port, tv);
  if (!c || c->err) {
    if (c)
      redisFree(c);
    return nullptr;
  }
  timeval rw{};
  rw.tv_sec = ep.rw_timeout_ms / 1000;
  rw.tv_usec = (ep.rw_timeout_ms % 1000) * 1000;
  redisSetTimeout(c, rw);
#ifdef HIREDIS_KEEPALIVE
  redisEnableKeepAlive(c);
#endif
  return c;
}

PipelinedBurst::PipelinedBurst(const Endpoint &ep, size_t connections,
                               size_t max_inflight_per_conn)
    : ep_(ep), max_inflight_per_conn_(max_inflight_per_conn) {
  if (connections == 0)
    connections = 1;
  conns_.reserve(connections);
  for (size_t i = 0; i < connections; ++i) {
    auto ptr = std::make_unique<Conn>();
    ptr->id = i;
    ptr->ctx = connectWithTimeout(ep_);
    conns_.emplace_back(std::move(ptr));
  }
}

PipelinedBurst::~PipelinedBurst() {
  for (auto &p : conns_) {
    if (p->ctx)
      redisFree(p->ctx);
    p->ctx = nullptr;
  }
}

bool PipelinedBurst::ensureConnected_(Conn &c) {
  if (c.ctx && c.ctx->err == 0)
    return true;
  if (c.ctx) {
    redisFree(c.ctx);
    c.ctx = nullptr;
  }
  c.ctx = connectWithTimeout(ep_);
  return c.ctx != nullptr;
}

PipelinedBurst::Conn *PipelinedBurst::pickConnForAppend_() {
  if (conns_.empty())
    return nullptr;
  const size_t n = conns_.size();
  for (size_t tries = 0; tries < n; ++tries) {
    Conn *c = conns_[(rr_++) % n].get();
    if (max_inflight_per_conn_ > 0 && c->inflight >= max_inflight_per_conn_) {
      continue; 
    }
    if (!ensureConnected_(*c)) {
      continue;
    }
    return c;
  }
  return nullptr;
}

// 有连接时间版本，若需要保持连接时间可以使用这个
bool PipelinedBurst::appendOne_(Conn &c, const BurstItem &item) {
  // 二进制安全（即便 payload 中包含空字符）
  int rc = redisAppendCommand(c.ctx, "MANIFESTINGEST %b", item.payload.data(),
                              (size_t)item.payload.size());
  if (rc != REDIS_OK) {
    return false;
  }

  // 尝试非阻塞 flush 一下，降低 obuf 积压（不强求写空）
  const int fd = c.ctx->fd;
  if (fd > 0) {
    struct pollfd wfd {};
    wfd.fd = fd;
    wfd.events = POLLOUT;
    if (poll(&wfd, 1, 0) > 0 && (wfd.revents & POLLOUT)) {
      int done = 0;
      (void)redisBufferWrite(c.ctx, &done);  // 清空缓冲区
    }
  }

  c.inflight += 1;
  c.tags.emplace_back(item.tag);
  LOG_INFO("[PipelinedBurst] Send success: tag=" + item.tag);
  return true;
}


// bool PipelinedBurst::appendOne_(Conn &c, const BurstItem &item) {
//     int rc = redisAppendCommand(c.ctx, "MANIFESTINGEST %b",
//                                 item.payload.data(), item.payload.size());
//     if (rc != REDIS_OK) return false;
//     int done = 0;
//     redisBufferWrite(c.ctx, &done);
//     LOG_INFO("[PipelinedBurst] Fire-and-forget send: tag=" + item.tag);
//     return true;
// }


bool PipelinedBurst::append(const BurstItem &item) {
  Conn *c = pickConnForAppend_();
  if (!c)
    return false;
  return appendOne_(*c, item);
}

size_t PipelinedBurst::appendAll(const std::vector<BurstItem> &items,
                                 std::vector<BurstItem> *rejected) {
  size_t accepted = 0;
  for (const auto &it : items) {
    Conn *c = pickConnForAppend_();
    if (!c) {
      LOG_WARN("[PipelinedBurst] all connections unavailable/full, drop append "
               "for tag=" +
               it.tag);
      if (rejected)
        rejected->push_back(it);
      continue;
    }
    if (appendOne_(*c, it)) {
      ++accepted;
    } else {
      LOG_WARN("[PipelinedBurst] append failed on chosen conn, drop tag=" +
               it.tag);
      if (rejected)
        rejected->push_back(it);
    }
  }
  return accepted;
}


std::optional<BurstResult> PipelinedBurst::getOneReply_(Conn &c) {
    if (!c.ctx || c.inflight == 0)
        return std::nullopt;

    const int fd = c.ctx->fd;
    if (fd <= 0) {
        BurstResult br;
        br.ok = false;
        br.err = "invalid socket fd";
        if (!c.tags.empty()) {
            br.tag = std::move(c.tags.front());
            c.tags.pop_front();
        }
        if (c.inflight > 0)
            c.inflight -= 1;
        while (!c.tags.empty()) {
            BurstResult x{false, "connection dropped", std::move(c.tags.front())};
            c.tags.pop_front();
            c.pending_failures.push_back(std::move(x));
        }
        c.inflight = 0;
        closeConn_(c);
        return br;
    }

    struct pollfd wfd {};
    wfd.fd = fd;
    wfd.events = POLLOUT;
    const int wready = poll(&wfd, 1, 0);
    if (wready < 0) {
        BurstResult br;
        br.ok = false;
        br.err = "poll POLLOUT error";
        if (!c.tags.empty()) {
            br.tag = std::move(c.tags.front());
            c.tags.pop_front();
        }
        if (c.inflight > 0)
            c.inflight -= 1;
        while (!c.tags.empty()) {
            c.pending_failures.push_back(BurstResult{false, "connection dropped", std::move(c.tags.front())});
            c.tags.pop_front();
        }
        c.inflight = 0;
        closeConn_(c);
        return br;
    }

    if (wready > 0 && (wfd.revents & POLLOUT)) {
        int done = 0;
        do {
            if (redisBufferWrite(c.ctx, &done) != REDIS_OK) {
                BurstResult br;
                br.ok = false;
                br.err = "redisBufferWrite error";
                if (!c.tags.empty()) {
                    br.tag = std::move(c.tags.front());
                    c.tags.pop_front();
                }
                if (c.inflight > 0)
                    c.inflight -= 1;
                while (!c.tags.empty()) {
                    c.pending_failures.push_back(BurstResult{
                        false, "connection dropped", std::move(c.tags.front())});
                    c.tags.pop_front();
                }
                c.inflight = 0;
                closeConn_(c);
                return br;
            }
        } while (!done);
    }

    struct pollfd rfd {};
    rfd.fd = fd;
    rfd.events = POLLIN;
    const int rready = poll(&rfd, 1, 0);
    if (rready < 0) {
        BurstResult br;
        br.ok = false;
        br.err = "poll POLLIN error";
        if (!c.tags.empty()) {
            br.tag = std::move(c.tags.front());
            c.tags.pop_front();
        }
        if (c.inflight > 0)
            c.inflight -= 1;
        while (!c.tags.empty()) {
            c.pending_failures.push_back(BurstResult{false, "connection dropped", std::move(c.tags.front())});
            c.tags.pop_front();
        }
        c.inflight = 0;
        closeConn_(c);
        return br;
    }

    if (rready == 0) {
        return std::nullopt;
    }

    void *rptr = nullptr;
    const int s = redisGetReply(c.ctx, &rptr);
    std::unique_ptr<redisReply, void (*)(void *)> reply(reinterpret_cast<redisReply *>(rptr), freeReplyObject);

    BurstResult br;
    if (!c.tags.empty()) {
        br.tag = std::move(c.tags.front());
        c.tags.pop_front();
    }
    if (c.inflight > 0)
        c.inflight -= 1;

    if (s != REDIS_OK) {
        br.ok = false;
        br.err = "redisGetReply error/timeout";
        while (!c.tags.empty()) {
            c.pending_failures.push_back(BurstResult{false, "connection dropped", std::move(c.tags.front())});
            c.tags.pop_front();
        }
        c.inflight = 0;
        closeConn_(c);
        return br;
    }
    if (!reply) {
        br.ok = false;
        br.err = "null reply";
        return br;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        br.ok = false;
        br.err = reply->str ? reply->str : "REDIS_REPLY_ERROR";
        return br;
    }
    br.ok = true;
    return br;
}


bool PipelinedBurst::drainPendingFailures_(Conn &c,
                                           std::vector<BurstResult> &out) {
  bool progressed = false;
  while (!c.pending_failures.empty()) {
    out.emplace_back(std::move(c.pending_failures.front()));
    c.pending_failures.pop_front();
    progressed = true;
  }
  return progressed;
}

std::vector<BurstResult>
PipelinedBurst::drainReplies(std::chrono::milliseconds max_wait,
                             bool force_quick_exit) {
  std::vector<BurstResult> out;
  if (inflight() == 0)
    return out;

  const auto deadline = std::chrono::steady_clock::now() + max_wait;

  while (inflight() > 0) {
    bool progressed = false;

    for (auto &p : conns_) {
      progressed |= drainPendingFailures_(*p, out);

      if (p->inflight == 0)
        continue;
      auto r = getOneReply_(*p);
      if (r.has_value()) {
        out.emplace_back(std::move(*r));
        progressed = true;
        progressed |= drainPendingFailures_(*p, out);
      }
    }

    if (!progressed) {
      if (force_quick_exit && inflight() == 0) {
        break;
      }
      if (std::chrono::steady_clock::now() >= deadline)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }

  return out;
}

std::vector<BurstResult>
PipelinedBurst::sendAndDrain(const std::vector<BurstItem> &items,
                             std::chrono::milliseconds max_wait) {
  appendAll(items);
  return drainReplies(max_wait);
}

size_t PipelinedBurst::inflight() const {
  size_t sum = 0;
  for (auto &p : conns_)
    sum += p->inflight;
  return sum;
}

void PipelinedBurst::closeConn_(Conn &c) {
  if (c.ctx) {
    redisFree(c.ctx);
    c.ctx = nullptr;
  }
}

} // namespace iagent
