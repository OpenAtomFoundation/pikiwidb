#ifndef S3PUT_MANIFEST_BUILDER_H
#define S3PUT_MANIFEST_BUILDER_H

#include <string>
#include <vector>
#include <future>
#include <cstdint>
#include "sstTracker.h"
#include "proto/manifest.pb.h"
#include <nlohmann/json.hpp> 
#ifndef _WIN32
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <filesystem>

namespace s3put
{
    namespace fs = std::filesystem;
    struct DirLock
    {
        int fd{-1};
        std::string path;
        bool ok{false};
        explicit DirLock(const std::string &manifest_dir)
        {
            path = (fs::path(manifest_dir) / ".build.lock").string();
#ifndef _WIN32
            fd = ::open(path.c_str(), O_CREAT | O_RDWR, 0666);
            if (fd >= 0)
            {
                if (::flock(fd, LOCK_EX | LOCK_NB) == 0)
                    ok = true;
            }
#else
            // Windows
            ok = true; 
#endif
        }
        ~DirLock()
        {
#ifndef _WIN32
            if (fd >= 0)
            {
                ::flock(fd, LOCK_UN);
                ::close(fd);
            }
#endif
        }
    };

    class ManifestBuilder
    {
    public:
        ManifestBuilder() = default;
        bool BuildAndWrite(const SstTracker &tracker,
                           size_t num_threads,
                           const std::string &manifest_dir,
                           const std::string &latest_path,
                           size_t max_per_part,
                           const std::string &version_id,
                           std::vector<std::string> *out_parts = nullptr);
        static std::string GenerateVersionId();
        static bool WriteLatestManifest(const std::string &path,
                                        const std::string &version_id,
                                        int64_t timestamp_ms,
                                        const std::vector<std::string> &manifest_files);

    private:
        static bool WriteManifestPart(const std::vector<s3put::manifest::SSTFile> &files,
                                      const std::string &manifest_file,
                                      const std::string &version_id);
    };

} // namespace s3put

#endif // S3PUT_MANIFEST_BUILDER_H
