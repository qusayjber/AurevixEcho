#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <sqlite3.h>

namespace aurevix {

class Database {
public:
    static Database& instance();

    bool open(const std::filesystem::path& path);
    void close();

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool exec(const std::string& sql);
    sqlite3* raw() { return db_; }
    std::mutex& mutex() { return m_; }

    // Runs migrations idempotently.
    bool migrate();

    // Wipes user data (keeps settings). Returns true on success.
    bool wipe_user_data();

    // Measures on-disk size in bytes.
    std::uint64_t file_size_bytes() const;

private:
    Database() = default;
    sqlite3* db_ = nullptr;
    std::mutex m_;
    std::filesystem::path path_;
};

} // namespace aurevix