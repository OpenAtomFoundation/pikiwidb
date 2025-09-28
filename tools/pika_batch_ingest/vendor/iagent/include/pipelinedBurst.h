#pragma once
#include <chrono>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct redisContext;
struct redisReply;

namespace iagent {
struct BurstResult {
  bool ok{true};
  std::string err;
  std::string tag; 
};

struct Endpoint {
  std::string host;
  int port{0};
  int connect_timeout_ms{3000}; 
  int rw_timeout_ms{5000};  
};

struct BurstItem {
  std::string payload; 
  std::string tag;  
};

class PipelinedBurst {
public:
  explicit PipelinedBurst(const Endpoint &ep, size_t connections = 4,
                          size_t max_inflight_per_conn = 0);
  ~PipelinedBurst();

  PipelinedBurst(const PipelinedBurst &) = delete;
  PipelinedBurst &operator=(const PipelinedBurst &) = delete;
  bool append(const BurstItem &item);
  size_t appendAll(const std::vector<BurstItem> &items,
                   std::vector<BurstItem> *rejected = nullptr);
  std::vector<BurstResult> drainReplies(std::chrono::milliseconds max_wait,
                                        bool force_quick_exit = true);
  std::vector<BurstResult> sendAndDrain(const std::vector<BurstItem> &items,
                                        std::chrono::milliseconds max_wait);
  size_t inflight() const;

  size_t connectionCount() const { return conns_.size(); }

private:
  struct Conn {
    redisContext *ctx{nullptr};
    size_t inflight{0}; 
    size_t id{0};
    std::deque<std::string> tags;
    std::deque<BurstResult> pending_failures;
  };

  bool ensureConnected_(Conn &c);
  bool appendOne_(Conn &c, const BurstItem &item);
  std::optional<BurstResult> getOneReply_(Conn &c);
  static bool drainPendingFailures_(Conn &c, std::vector<BurstResult> &out);
  Conn *pickConnForAppend_();
  static void closeConn_(Conn &c);

private:
  Endpoint ep_;
  std::vector<std::unique_ptr<Conn>> conns_;
  size_t rr_{0}; 
  size_t max_inflight_per_conn_{0};
};

} // namespace iagent
