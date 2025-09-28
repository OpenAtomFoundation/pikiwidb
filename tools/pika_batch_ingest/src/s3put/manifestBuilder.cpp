// manifestBuilder.cpp
#include "manifestBuilder.h"
#include "sstTracker.h"
#include "utils/klog.h"

#include <filesystem>
#include <fstream>
#include <future>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <limits>

#include <nlohmann/json.hpp>
#include "manifest.pb.h"

namespace fs = std::filesystem;

namespace s3put
{
    namespace
    {

        inline int64_t NowMs()
        {
            using namespace std::chrono;
            return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }
        inline bool AtomicWriteText(const std::string &path,
                                    const std::string &body,
                                    std::string *err = nullptr)
        {
            std::error_code ec;
            fs::path p(path);
            if (p.has_parent_path())
            {
                fs::create_directories(p.parent_path(), ec);
                if (ec)
                {
                    if (err)
                        *err = "create_directories failed: " + ec.message();
                    return false;
                }
            }
            const std::string tmp = path + ".tmp";
            {
                std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
                if (!ofs.is_open())
                {
                    if (err)
                        *err = "open tmp failed: " + tmp;
                    return false;
                }
                ofs.write(body.data(), static_cast<std::streamsize>(body.size()));
                ofs.flush();
                if (!ofs)
                {
                    if (err)
                        *err = "flush tmp failed: " + tmp;
                    return false;
                }
            }
            fs::rename(tmp, path, ec);
            if (ec)
            {
                if (err)
                    *err = "rename failed: " + ec.message();
                return false;
            }
            return true;
        }

        inline bool AtomicWriteProto(const std::string &path,
                                     const google::protobuf::Message &msg,
                                     std::string *err = nullptr)
        {
            std::error_code ec;
            fs::path p(path);
            if (p.has_parent_path())
            {
                fs::create_directories(p.parent_path(), ec);
                if (ec)
                {
                    if (err)
                        *err = "create_directories failed: " + ec.message();
                    return false;
                }
            }
            const std::string tmp = path + ".tmp";
            {
                std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
                if (!ofs.is_open())
                {
                    if (err)
                        *err = "open tmp failed: " + tmp;
                    return false;
                }
                if (!msg.SerializeToOstream(&ofs))
                {
                    if (err)
                        *err = "SerializeToOstream failed";
                    return false;
                }
                ofs.flush();
                if (!ofs)
                {
                    if (err)
                        *err = "flush tmp failed: " + tmp;
                    return false;
                }
            }
            fs::rename(tmp, path, ec);
            if (ec)
            {
                if (err)
                    *err = "rename failed: " + ec.message();
                return false;
            }
            return true;
        }

        inline void SortPartsByNumericIndex(std::vector<std::string> &parts)
        {
            auto key = [](const std::string &p) -> int
            {
                auto pos = p.rfind("_part");
                if (pos == std::string::npos)
                    return -1;
                auto end = p.find('.', pos);
                if (end == std::string::npos)
                    end = p.size();
                try
                {
                    return std::stoi(p.substr(pos + 5, end - (pos + 5)));
                }
                catch (...)
                {
                    return -1;
                }
            };
            std::sort(parts.begin(), parts.end(), [&](const std::string &a, const std::string &b)
                      { return key(a) < key(b); });
        }

        std::mutex g_mu;
        std::unordered_set<std::string> g_building_versions;
        std::unordered_set<std::string> g_writing_parts; 

        struct BuildGuard
        {
            std::string version;
            bool active{false};
            BuildGuard(const std::string &v) : version(v)
            {
                std::lock_guard<std::mutex> lk(g_mu);
                auto ok = g_building_versions.insert(version).second;
                active = ok;
            }
            ~BuildGuard()
            {
                if (!active)
                    return;
                std::lock_guard<std::mutex> lk(g_mu);
                g_building_versions.erase(version);
            }
            bool ok() const { return active; }
        };

    } // anonymous namespace

#ifdef _WIN32
#include <process.h>
    static inline int get_pid() { return _getpid(); }
#else
#include <unistd.h>
    static inline int get_pid() { return static_cast<int>(::getpid()); }
#endif

    static std::atomic<uint64_t> g_version_seq{0};

    std::string ManifestBuilder::GenerateVersionId()
    {
        using namespace std::chrono;
        const uint64_t ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        const uint64_t pid = static_cast<uint64_t>(get_pid()) % 1000ULL;                   
        const uint64_t seq = g_version_seq.fetch_add(1, std::memory_order_relaxed) % 1000ULL;
        const uint64_t ver = ms * 1000000ULL + pid * 1000ULL + seq;       
        return std::to_string(ver);
    }

    bool ManifestBuilder::WriteManifestPart(
        const std::vector<s3put::manifest::SSTFile> &files,
        const std::string &path,
        const std::string &version_id)
    {
        s3put::manifest::Manifest part;
        part.set_version_id(version_id);
        part.set_timestamp(NowMs());
        for (const auto &f : files)
        {
            *part.add_sst_files() = f; 
        }
        std::string err;
        if (!AtomicWriteProto(path, part, &err))
        {
            LOG_ERROR("WriteManifestPart: " + err);
            return false;
        }
        LOG_INFO("Wrote manifest part: " + path);
        return true;
    }

    bool ManifestBuilder::WriteLatestManifest(const std::string &path,
                                              const std::string &version_id,
                                              int64_t timestamp_ms,
                                              const std::vector<std::string> &manifest_files)
    {
        nlohmann::json j;
        j["version_id"] = version_id;
        j["timestamp_ms"] = timestamp_ms;
        j["parts"] = manifest_files;

        std::string err;
        if (!AtomicWriteText(path, j.dump(2), &err))
        {
            LOG_ERROR("WriteLatestManifest: " + err);
            return false;
        }
        LOG_INFO("latest.manifest updated: " + path + " parts=" + std::to_string(manifest_files.size()));
        return true;
    }

    bool ManifestBuilder::BuildAndWrite(const SstTracker &tracker,
                                        size_t num_threads,
                                        const std::string &manifest_dir,
                                        const std::string &latest_path,
                                        size_t max_per_part,
                                        const std::string &version_id,
                                        std::vector<std::string> *out_parts)
    {
        BuildGuard guard(version_id);
        if (!guard.ok())
        {
            LOG_WARN("BuildAndWrite: duplicate invocation for version " + version_id + ", skip.");
            return false;
        }

        DirLock lock(manifest_dir);
        if (!lock.ok)
        {
            LOG_WARN("BuildAndWrite: another process is building in " + manifest_dir + ", skip.");
            if (out_parts)
                out_parts->clear();
            return true; 
        }

        if (version_id.empty())
        {
            LOG_ERROR("BuildAndWrite: version_id is empty.");
            return false;
        }
        if (num_threads == 0)
            num_threads = 1;
        if (max_per_part == 0)
            max_per_part = std::numeric_limits<size_t>::max();

        std::error_code ec;
        fs::create_directories(manifest_dir, ec);
        if (ec)
        {
            LOG_ERROR("BuildAndWrite: create_directories failed: " + ec.message());
            return false;
        }

        std::vector<std::string> files = tracker.GetChangedFiles();
        if (files.empty())
        {
            LOG_WARN("BuildAndWrite: no changed files; nothing to write.");
            if (out_parts)
                out_parts->clear();
            return true;
        }
        LOG_INFO("BuildAndWrite start: version=" + version_id + ", files=" + std::to_string(files.size()));

        struct Entry
        {
            std::string sst_path;
            std::string hash;
            long long size;
        };
        std::vector<Entry> entries(files.size());

        const size_t workers = std::max<size_t>(1, num_threads);
        std::atomic<size_t> next{0};
        std::vector<std::future<void>> futs;
        futs.reserve(workers);
        for (size_t t = 0; t < workers; ++t)
        {
            futs.emplace_back(std::async(std::launch::async, [&]
                                         {
            for (;;) {
                size_t i = next.fetch_add(1, std::memory_order_relaxed);
                if (i >= files.size()) break;
                Entry e;
                e.sst_path = tracker.GenerateSstUploadKey(files[i]);
                e.hash     = tracker.GetFileHash(files[i]);
                e.size     = tracker.GetFileSize(files[i]);
                entries[i] = std::move(e);
            } }));
        }
        for (auto &f : futs)
            f.get();

        const size_t per = std::max<size_t>(1, max_per_part);
        const size_t num_parts = (entries.size() + per - 1) / per;

        std::vector<std::string> part_paths(num_parts);
        for (size_t part_no = 0; part_no < num_parts; ++part_no)
        {
            part_paths[part_no] = (fs::path(manifest_dir) /
                                   ("manifest_" + version_id + "_part" + std::to_string(part_no) + ".proto"))
                                      .string();
        }

        std::vector<std::future<std::pair<bool, std::string>>> writers;
        writers.reserve(num_parts);

        for (size_t part_no = 0; part_no < num_parts; ++part_no)
        {
            const size_t start = part_no * per;
            const size_t end = std::min(start + per, entries.size());
            const std::string mf = part_paths[part_no];

            {
                std::lock_guard<std::mutex> lk(g_mu);
                if (!g_writing_parts.insert(mf).second)
                {
                    LOG_WARN("BuildAndWrite: part already in-flight, skip: " + mf);
                    continue;
                }
            }

            writers.emplace_back(std::async(std::launch::async,
                                            [&, part_no, start, end, mf]() -> std::pair<bool, std::string>
                                            {
                                                std::vector<s3put::manifest::SSTFile> chunk;
                                                chunk.reserve(end - start);
                                                for (size_t i = start; i < end; ++i)
                                                {
                                                    s3put::manifest::SSTFile f;
                                                    f.set_sst_path(entries[i].sst_path);
                                                    f.set_hash(entries[i].hash);
                                                    f.set_file_size(entries[i].size);
                                                    chunk.push_back(std::move(f));
                                                }

                                                bool ok = WriteManifestPart(chunk, mf, version_id);

                                                {
                                                    std::lock_guard<std::mutex> lk(g_mu);
                                                    g_writing_parts.erase(mf);
                                                }

                                                if (!ok)
                                                {
                                                    LOG_ERROR("WriteManifestPart failed: " + mf);
                                                }
                                                return {ok, mf};
                                            }));
        }

        std::vector<std::string> manifest_files;
        std::vector<std::string> manifest_files_abs;

        manifest_files.reserve(writers.size());
        bool all_ok = true;
        for (auto &w : writers)
        {
            auto [ok, path] = w.get();
            if (ok)
            {
                manifest_files.push_back(fs::path(path).filename().string());
                manifest_files_abs.push_back(path);
            }
            else
                all_ok = false;
        }
        if (!all_ok)
            return false;

        SortPartsByNumericIndex(manifest_files);
        manifest_files.erase(std::unique(manifest_files.begin(), manifest_files.end()),
                             manifest_files.end());

        const int64_t ts = NowMs();
        if (!WriteLatestManifest(latest_path, version_id, ts, manifest_files))
        {
            LOG_ERROR("BuildAndWrite: WriteLatestManifest failed.");
            return false;
        }

        if (out_parts)
            *out_parts = manifest_files_abs;

        LOG_INFO("Manifest build completed. version=" + version_id +
                 ", parts=" + std::to_string(manifest_files.size()) +
                 ", files=" + std::to_string(files.size()));

        return true;
    }

} // namespace s3put
