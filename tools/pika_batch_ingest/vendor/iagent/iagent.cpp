#include "agentRunner.h"
#include "utils/kconfig.h"
#include "utils/threadScheduler.h"
#include "utils/klog.h"

int main() {
  using namespace iagent;
  
  try {
    initThreadSchedulerFromConfig(IAGENTTHREADCONF);

    auto s3Config = ConfigLoader::loadS3Config(IAGENTS3CONFIG);
    auto pikaConfig = ConfigLoader::loadPikaConfig(IAGENTPIKACONFIG);
    AgentRunner runner(s3Config, pikaConfig, IAGENTMANIFESTQUE,
                       IAGENTMANIFESTOFFSET);
    runner.run();
  } catch (const std::exception& e) {
    LOG_ERROR(std::string("iagent failed to start: ") + e.what());
    return 1;
  }

  return 0;
}
