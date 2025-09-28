// iagent/AgentRunner.h
#pragma once

#include <string>
#include <atomic>
#include "configLoader.h"
#include "s3Fetcher.h"
#include "manifestWatcher.h"

namespace iagent
{

    class AgentRunner
    {
    public:
        AgentRunner(const S3Config &s3Conf,
                    const PikaConfig &pikaConf,
                    const std::string &queuePath,
                    const std::string &offsetPath);

        void run();

    private:
        void loopOnce();
        void stop();
        void setupSignalHandlers();

        std::atomic<bool> running_;
        S3Config s3Config_;
        PikaConfig pikaConfig_;
        S3Fetcher fetcher_;
        ManifestWatcher watcher_;
    };

} // namespace iagent
