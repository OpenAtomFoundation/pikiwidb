#include "include/agentRunner.h"
#include "include/configLoader.h"
#include "include/pipelinedBurst.h"
#include "include/s3Fetcher.h"
#include "utils/klog.h"
#include "utils/ktime.h"
#include "utils/threadScheduler.h"

#include <chrono>
#include <csignal>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

namespace iagent {

static AgentRunner *globalRunner = nullptr;

AgentRunner::AgentRunner(const S3Config &s3Conf, const PikaConfig &pikaConf,
                         const std::string &queuePath,
                         const std::string &offsetPath)
    : running_(true), s3Config_(s3Conf), pikaConfig_(pikaConf),
      fetcher_(s3Conf), watcher_(queuePath, offsetPath) {
  globalRunner = this;
  setupSignalHandlers();
}

void AgentRunner::setupSignalHandlers() {
  std::signal(SIGINT, [](int) {
    LOG_DEBUG("[AgentRunner] SIGINT received, shutting down...");
    if (globalRunner)
      globalRunner->stop();
  });
  std::signal(SIGTERM, [](int) {
    LOG_DEBUG("[AgentRunner] SIGTERM received, shutting down...");
    if (globalRunner)
      globalRunner->stop();
  });
}

void AgentRunner::stop() {
  running_ = false; 
  fetcher_.stop(); 
}

void AgentRunner::run() {
  LOG_INFO("[Agent] Started.");
  TimeTracker::Start("[@IAGENT]");

  fetcher_.start([this](const std::string &entry) {
    if (!entry.empty()) watcher_.enqueue(entry);
  });

  Endpoint ep{pikaConfig_.host, pikaConfig_.port,
              s3Config_.connect_timeout_ms, s3Config_.rw_timeout_ms};
  size_t conns = std::max<size_t>(1, ThreadScheduler::get().get("pipe_conns"));
  PipelinedBurst burst(ep, conns);

  const size_t BATCH_SIZE = s3Config_.manifest_batch;
  size_t inflight_total = 0; 

  size_t retry_count = 0;    
  while (running_ || watcher_.hasPending() || inflight_total > 0) {
    if (!watcher_.hasPending() && inflight_total == 0) {
      if (retry_count >= s3Config_.max_retries) {
        LOG_INFO("[Agent] No more tasks and max retries reached. Exiting.");
        break;
      } else {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        retry_count++;
        continue;
      }
    }

    std::vector<std::string> keys;
    keys.reserve(BATCH_SIZE);
    for (size_t i = 0; i < BATCH_SIZE; ++i) {
      if (!watcher_.hasPending()) break;
      std::string k = watcher_.popNext();
      if (k.empty()) break;
      keys.emplace_back(std::move(k));
    }

    if (!keys.empty()) {
      std::vector<BurstItem> items;
      items.reserve(keys.size());
      for (auto &key : keys) items.push_back(BurstItem{/*payload=*/key, /*tag=*/key});

      std::vector<BurstItem> rejected;
      size_t accepted = burst.appendAll(items, &rejected);

      for (auto &it : rejected) watcher_.enqueue(it.tag);
      if (!rejected.empty()) {
        LOG_WARN("[Agent] requeued " + std::to_string(rejected.size()) + " items");
      }

      inflight_total += accepted;  
      watcher_.ack(items.size());       
    }

   
     // —— drain replies —— 
    auto replies = burst.drainReplies(std::chrono::milliseconds(500), true);

     if (!replies.empty()) {
        size_t okCnt = 0, failCnt = 0;
        for (auto &r : replies) {
            if (r.ok) {
                ++okCnt;
            } else {
                ++failCnt;
                LOG_WARN("[Agent] fail tag=" + r.tag + " err=" + r.err);
            }
        }
        inflight_total -= replies.size();
        // LOG_INFO("[Agent] drain: ok=" + std::to_string(okCnt) +
        //          " fail=" + std::to_string(failCnt) +
        //          " inflight=" + std::to_string(inflight_total));
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // —— 任务完成立即退出 —— 
    if (!watcher_.hasPending() && inflight_total == 0) {
        LOG_INFO("[Agent] All tasks completed. Exiting.");
        break;
    }
  
     // 没有 drain，直接检查队列
    // if (!watcher_.hasPending()) {
    //     LOG_INFO("[Agent] All tasks completed. Exiting.");
    //     break;
    // }
  }

  TimeTracker::End();
  LOG_INFO("[Agent] Clean exit.");
}

} // namespace iagent
