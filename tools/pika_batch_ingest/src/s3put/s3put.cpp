#include "configManager.h"
#include "manifestBuilder.h"
#include "s3SyncManager.h"
#include "utils/kconfig.h"
#include "utils/klog.h"
#include <aws/core/Aws.h>

int main(int argc, char *argv[]) {
  s3put::S3SyncManager manager;
  if (!s3put::ConfigManager::getInstance().loadConfig(S3CONFIG)) {
    LOG_ERROR("Failed to load config file: " + S3CONFIG.string());
    return 1;
  }
  if (!manager.Init(S3CONFIG)) {
    LOG_ERROR("Failed to initialize S3SyncManager with config and SST dir");
    return 1;
  }

  manager.Run();
  return 0;
}
