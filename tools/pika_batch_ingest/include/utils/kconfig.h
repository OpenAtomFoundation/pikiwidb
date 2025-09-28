

#ifndef KCONFIG_H
#define KCONFIG_H
#include <filesystem>
#ifndef PROJECT_DIR
#define PROJECT_DIR "data"
#endif

#ifndef CONFIG_DIR
#define CONFIG_DIR "config"
#endif

// mock
const std::filesystem::path DEFAULTDIC =
    std::filesystem::path(PROJECT_DIR) / "mock";
const std::filesystem::path MOCKTHREADCONF =
    std::filesystem::path(CONFIG_DIR) / "mock_threads.json";

// exchange
const std::filesystem::path DEFAULTSSTDIC =
    std::filesystem::path(PROJECT_DIR) / "sst";
const std::filesystem::path DEFUALTSSTTEST =
    std::filesystem::path(DEFAULTSSTDIC) / "test-10M";
const std::filesystem::path SUMMERMETA =
    std::filesystem::path(CONFIG_DIR) / "sst_count.json";

// s3put
const std::filesystem::path MANIFESTDIC =
    std::filesystem::path(PROJECT_DIR) / "manifest";
const std::filesystem::path STATUSTDIC =
    std::filesystem::path(PROJECT_DIR) / "status";
const std::filesystem::path LASTMANIFEST =
    std::filesystem::path(MANIFESTDIC) / "last.manifest";
const std::filesystem::path DEFAULTCONFIGFILEDIC =
    std::filesystem::path(CONFIG_DIR) / "dics.json";
const std::filesystem::path HASHTABLE =
    std::filesystem::path(CONFIG_DIR) / "hashTable.json";
const std::filesystem::path S3CONFIG =
    std::filesystem::path(CONFIG_DIR) / "s3_config.json";

// iagent
const std::filesystem::path IAGENTS3CONFIG =
    std::filesystem::path(CONFIG_DIR) / "iagent.json";
const std::filesystem::path IAGENTPIKACONFIG =
    std::filesystem::path(CONFIG_DIR) / "pika.json";
const std::filesystem::path IAGENTMANIFESTQUE =
    std::filesystem::path(CONFIG_DIR) / "manifest.queue";
const std::filesystem::path IAGENTMANIFESTOFFSET =
    std::filesystem::path(CONFIG_DIR) / "manifest.offset";
const std::filesystem::path IAGENTTHREADCONF =
    std::filesystem::path(CONFIG_DIR) / "iagent_threads.json";

// time
const std::filesystem::path TIMEDICT =
    std::filesystem::path(PROJECT_DIR) / "time";
const std::filesystem::path TIEMRECORDPATH =
    std::filesystem::path(TIMEDICT) / "time_record.csv";

//log
const std::filesystem::path KLOGDICT =
    std::filesystem::path(PROJECT_DIR) / "klog";
const std::filesystem::path KLOGPATH = KLOGDICT / "klog.txt";
#endif