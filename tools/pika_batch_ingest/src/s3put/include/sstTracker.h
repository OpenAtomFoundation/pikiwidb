#ifndef S3PUT_SSTTRACKER_H
#define S3PUT_SSTTRACKER_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <vector>
#include <mutex>
#include <filesystem>
#include "proto/manifest.pb.h"

namespace s3put
{

    class SstTracker
    {
        using Sha256 = std::array<uint8_t, 32>;

    public:
        SstTracker() = default;

        void SetSstRoot(const std::string &root);
        void SetKeyPrefix(const std::string &prefix);
        void SetCurrentVersionId(const std::string &ver);

        void SetHashVerifyOnUnchanged(bool on) { hash_verify_on_unchanged_ = on; }
        bool GetHashVerifyOnUnchanged() const { return hash_verify_on_unchanged_; }

        bool HasChanged(const std::string &filepath);

        void ClearChanged();
        std::vector<std::string> GetChangedFiles() const;
        void ReplaceChanged(const std::vector<std::string> &files);
        void AddChanged(const std::string &file);

        std::string GenerateSstUploadKey(const std::string &abs_path) const;
        long long GetFileSize(const std::string &path) const;
        std::string GetFileHash(const std::string &path) const;
        static Sha256 ComputeSha256(const std::string &path);
        static std::string ShaToHex(const Sha256 &hash);

        void ExportToManifest(s3put::manifest::Manifest &manifest) const;

        bool LoadState(const std::string &path);
        bool SaveState(const std::string &path) const;
        int GetCurrentStatus(const std::string &filepath) const;
        void SetStatus(const std::string &filepath, int code);

    private:
        mutable std::mutex mu_;

        std::string sst_root_;
        std::string key_prefix_;
        std::string current_version_id_;

        struct FileMeta
        {
            long long size{-1};
            int64_t mtime_sec;
            std::string hash_hex;
            int status = 0; 
        };

        std::unordered_map<std::string, FileMeta> known_;
        std::unordered_set<std::string> changed_files_;

        bool hash_verify_on_unchanged_{true};

        std::string ExtractDictFromPath(const std::string &abs_path) const;
    };

} // namespace s3put

#endif // S3PUT_SSTTRACKER_H
