#include "sstTracker.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include "utils/klog.h"
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
namespace s3put
{

    void SstTracker::SetSstRoot(const std::string &root)
    {
        sst_root_ = root;
    }
    void SstTracker::SetKeyPrefix(const std::string &prefix)
    {
        key_prefix_ = prefix;
    }
    void SstTracker::SetCurrentVersionId(const std::string &ver)
    {
        current_version_id_ = ver;
    }

    static inline std::time_t ToTimeT(const std::filesystem::file_time_type &ft)
    {
        using namespace std::chrono;
        return system_clock::to_time_t(time_point_cast<system_clock::duration>(
            ft - std::filesystem::file_time_type::clock::now() + system_clock::now()));
    }

    bool SstTracker::SaveState(const std::string &path) const
    {
        nlohmann::json j;
        {
            std::lock_guard<std::mutex> lk(mu_);
            for (const auto &[fp, meta] : known_)
            {
                nlohmann::json o;
                o["size"] = meta.size;
                o["mtime_sec"] = static_cast<int64_t>(meta.mtime_sec);
                o["hash"] = meta.hash_hex;
                o["status"] = meta.status; 
                j[fp] = std::move(o);
            }
        }

        const std::string tmp = path + ".tmp";
        {
            std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
            if (!ofs.is_open())
            {
                LOG_ERROR("SaveState: open failed: " + tmp);
                return false;
            }
            ofs << j.dump(2);
            ofs.flush();
            if (!ofs)
            {
                LOG_ERROR("SaveState: write failed: " + tmp);
                return false;
            }
        }
        fs::rename(tmp, path);
        return true;
    }

    bool SstTracker::LoadState(const std::string &path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open())
        {
            LOG_WARN("LoadState: no state file: " + path);
            return false;
        }

        nlohmann::json j;
        try
        {
            ifs >> j;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR(std::string("LoadState: parse json failed: ") + e.what());
            return false;
        }

        std::lock_guard<std::mutex> lk(mu_);
        known_.clear();

        for (auto it = j.begin(); it != j.end(); ++it)
        {
            try
            {
                const std::string fp = it.key();
                const auto &o = it.value();

                FileMeta m;
                m.size = o.value("size", -1ll);
                m.mtime_sec = o.value("mtime_sec", int64_t{0});
                m.hash_hex = o.value("hash", "");
                m.status = o.value("status", 0); 

                known_.emplace(fp, std::move(m));
            }
            catch (const std::exception &e)
            {
                LOG_WARN("LoadState: skip bad entry for " + it.key() + " (" + e.what() + ")");
            }
        }

        LOG_INFO("LoadState: restored " + std::to_string(known_.size()) + " entries");
        return true;
    }

    int SstTracker::GetCurrentStatus(const std::string &filepath) const
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = known_.find(filepath);
        return (it != known_.end()) ? it->second.status : 0;
    }

    void SstTracker::SetStatus(const std::string &filepath, int code)
    {
        std::lock_guard<std::mutex> lk(mu_);
        FileMeta &meta = known_[filepath]; 
        meta.status = code;
    }

    bool SstTracker::HasChanged(const std::string &filepath)
    {
        std::error_code ec;
        fs::file_time_type ft = fs::last_write_time(filepath, ec);
        long long size = ec ? -1 : static_cast<long long>(fs::file_size(filepath, ec));
        if (ec || size < 0)
        {
            LOG_ERROR("HasChanged: stat failed for " + filepath);
            return false;
        }

        const int64_t mtime_sec = static_cast<int64_t>(ToTimeT(ft));
        bool need_hash = false;
        bool seen_before = false;
        {
            std::lock_guard<std::mutex> lk(mu_);
            auto it = known_.find(filepath);
            if (it == known_.end())
            {
                need_hash = true;
            }
            else
            {
                seen_before = true;
                if (it->second.status == 1)
                {
                    return true;
                }
                const bool meta_unchanged = (it->second.size == size) && (it->second.mtime_sec == mtime_sec);
                if (meta_unchanged)
                {
                    if (!hash_verify_on_unchanged_)
                    {
                        return false;
                    }
                    need_hash = true;
                }
                else
                {
                    need_hash = true;
                }
            }
        }
        std::string cur_hash_hex;
        if (need_hash)
            cur_hash_hex = ShaToHex(ComputeSha256(filepath));
        bool changed = false;
        {
            std::lock_guard<std::mutex> lk(mu_);
            FileMeta &meta = known_[filepath];

            std::error_code ec2;
            fs::file_time_type ft2 = fs::last_write_time(filepath, ec2);
            long long size2 = ec2 ? -1 : static_cast<long long>(fs::file_size(filepath, ec2));
            if (ec2 || size2 < 0)
            {
                LOG_ERROR("HasChanged: restat failed for " + filepath);
                return false;
            }
            const int64_t mtime_sec2 = static_cast<int64_t>(ToTimeT(ft2));

            const bool meta_changed_now = (!seen_before) ||
                                          (meta.size != size2) ||
                                          (meta.mtime_sec != mtime_sec2) ||
                                          meta.hash_hex.empty();

            bool hash_changed_now = false;
            if (!cur_hash_hex.empty())
            {
                hash_changed_now = meta.hash_hex.empty() || (meta.hash_hex != cur_hash_hex);
            }

            changed = (meta_changed_now || hash_changed_now);
            meta.size = size2;
            meta.mtime_sec = mtime_sec2;
            if (!cur_hash_hex.empty())
                meta.hash_hex = cur_hash_hex;
            if (changed)
            {
                changed_files_.insert(filepath);
            }
        }

        return changed;
    }

    void SstTracker::ClearChanged()
    {
        std::lock_guard<std::mutex> lk(mu_);
        changed_files_.clear();
    }

    std::vector<std::string> SstTracker::GetChangedFiles() const
    {
        std::lock_guard<std::mutex> lk(mu_);
        return {changed_files_.begin(), changed_files_.end()};
    }

    void SstTracker::ReplaceChanged(const std::vector<std::string> &files)
    {
        std::lock_guard<std::mutex> lk(mu_);
        changed_files_.clear();
        changed_files_.insert(files.begin(), files.end());
    }

    void SstTracker::AddChanged(const std::string &file)
    {
        std::lock_guard<std::mutex> lk(mu_);
        changed_files_.insert(file);
    }

    void SstTracker::ExportToManifest(s3put::manifest::Manifest &manifest) const
    {
        std::vector<std::string> snapshot;
        {
            std::lock_guard<std::mutex> lk(mu_);
            snapshot.assign(changed_files_.begin(), changed_files_.end());
        }
        for (const auto &file : snapshot)
        {
            auto *sst_file = manifest.add_sst_files();
            sst_file->set_sst_path(GenerateSstUploadKey(file));
            sst_file->set_hash(GetFileHash(file));
            sst_file->set_file_size(GetFileSize(file));
        }
    }

    static inline std::string NormalizeSlashes(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    static inline std::string SanitizeToken(std::string s)
    {
        for (char &c : s)
        {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.'))
                c = '_';
        }
        return s;
    }

    std::string SstTracker::GenerateSstUploadKey(const std::string &abs_path) const
    {
        if (sst_root_.empty() || key_prefix_.empty() || current_version_id_.empty())
        {
            LOG_ERROR("GenerateSstUploadKey: sst_root_, key_prefix_, or version_id not set!");
            return "";
        }

        std::string prefix = key_prefix_;
        if (!prefix.empty() && (prefix.back() == '/' || prefix.back() == '\\'))
            prefix.pop_back();

        fs::path p(abs_path);
        std::string stem = p.stem().string();     
        std::string ext = p.extension().string(); 

        if (stem.empty())
        {
            stem = p.filename().string();
            auto dot = stem.rfind('.');
            if (dot != std::string::npos)
                stem = stem.substr(0, dot);
        }
        if (ext != ".sst")
            ext = ".sst";

        std::string dict = ExtractDictFromPath(abs_path);
        if (dict.empty())
            dict = "unknown";

        stem = SanitizeToken(stem);
        dict = SanitizeToken(dict);

        std::string filename = stem + "_" + key_prefix_ + "_" + current_version_id_ + ext;
        filename = NormalizeSlashes(filename);

        return "sst/" + dict + "/" + filename;
    }

    long long SstTracker::GetFileSize(const std::string &path) const
    {
        std::error_code ec;
        auto sz = static_cast<long long>(fs::file_size(path, ec));
        return ec ? -1 : sz;
    }

    std::string SstTracker::GetFileHash(const std::string &path) const
    {
        Sha256 bin = ComputeSha256(path);
        return ShaToHex(bin);
    }

    SstTracker::Sha256 SstTracker::ComputeSha256(const std::string &path)
    {
        Sha256 hash{};
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open())
        {
            LOG_ERROR("ComputeSha256: cannot open file " + path);
            return hash;
        }

        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        if (!ctx)
        {
            LOG_ERROR("ComputeSha256: EVP_MD_CTX_new failed");
            return hash;
        }
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
        {
            LOG_ERROR("ComputeSha256: EVP_DigestInit_ex failed");
            EVP_MD_CTX_free(ctx);
            return hash;
        }

        const size_t BUF_SIZE = 1 << 20;
        std::vector<char> buffer(BUF_SIZE);
        while (ifs.good())
        {
            ifs.read(buffer.data(), buffer.size());
            std::streamsize n = ifs.gcount();
            if (n > 0)
            {
                if (EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(n)) != 1)
                {
                    LOG_ERROR("ComputeSha256: EVP_DigestUpdate failed");
                    EVP_MD_CTX_free(ctx);
                    return hash;
                }
            }
        }
        unsigned int out_len = 0;
        if (EVP_DigestFinal_ex(ctx, hash.data(), &out_len) != 1 || out_len != hash.size())
        {
            LOG_ERROR("ComputeSha256: EVP_DigestFinal_ex failed");
        }
        EVP_MD_CTX_free(ctx);
        return hash;
    }

    std::string SstTracker::ShaToHex(const Sha256 &hash)
    {
        static const char *hex = "0123456789abcdef";
        std::string out;
        out.reserve(hash.size() * 2);
        for (auto c : hash)
        {
            out.push_back(hex[(c >> 4) & 0xF]);
            out.push_back(hex[c & 0xF]);
        }
        return out;
    }

    std::string SstTracker::ExtractDictFromPath(const std::string &abs_path) const
    {
        fs::path p(abs_path);
        if (p.has_parent_path())
        {
            return p.parent_path().filename().string();
        }
        return "";
    }

} // namespace s3put
