#include "storage/Database.h"
#include "core/Logger.h"

namespace aurevix {

Database& Database::instance() {
    static Database inst;
    return inst;
}

bool Database::open(const std::filesystem::path& path) {
    std::lock_guard lock(m_);
    path_ = path;
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(path.string().c_str(), &db_, flags, nullptr) != SQLITE_OK) {
        AUR_ERROR("sqlite3_open_v2 failed: " << sqlite3_errmsg(db_));
        return false;
    }
    sqlite3_busy_timeout(db_, 5000);
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA synchronous=NORMAL;");
    exec("PRAGMA foreign_keys=ON;");
    AUR_INFO("Database opened at " << path.string());
    return true;
}

void Database::close() {
    std::lock_guard lock(m_);
    if (db_) { sqlite3_close(db_); db_ = nullptr; }
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        AUR_ERROR("SQL error: " << (err ? err : "unknown"));
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool Database::migrate() {
    std::lock_guard lock(m_);
    // Version table
    exec("CREATE TABLE IF NOT EXISTS schema_version(version INTEGER NOT NULL);");
    // Read current version
    int version = 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT version FROM schema_version LIMIT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) version = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    auto set_version = [&](int v) {
        exec("DELETE FROM schema_version;");
        std::string sql = "INSERT INTO schema_version(version) VALUES(" + std::to_string(v) + ");";
        exec(sql);
    };

    if (version < 1) {
        exec(R"SQL(
            CREATE TABLE IF NOT EXISTS applications(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                category TEXT,
                first_seen INTEGER NOT NULL,
                last_seen INTEGER NOT NULL,
                total_seconds INTEGER NOT NULL DEFAULT 0
            );

            CREATE TABLE IF NOT EXISTS activity_events(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                application_id INTEGER NOT NULL,
                started_at INTEGER NOT NULL,
                ended_at INTEGER,
                duration_seconds INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY(application_id) REFERENCES applications(id)
            );
            CREATE INDEX IF NOT EXISTS idx_activity_started ON activity_events(started_at);

            CREATE TABLE IF NOT EXISTS sessions(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                application_id INTEGER NOT NULL,
                started_at INTEGER NOT NULL,
                ended_at INTEGER NOT NULL,
                duration_seconds INTEGER NOT NULL,
                FOREIGN KEY(application_id) REFERENCES applications(id)
            );
            CREATE INDEX IF NOT EXISTS idx_session_started ON sessions(started_at);

            CREATE TABLE IF NOT EXISTS transitions(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                from_app INTEGER NOT NULL,
                to_app INTEGER NOT NULL,
                at INTEGER NOT NULL,
                FOREIGN KEY(from_app) REFERENCES applications(id),
                FOREIGN KEY(to_app)   REFERENCES applications(id)
            );
            CREATE INDEX IF NOT EXISTS idx_transition_at ON transitions(at);

            CREATE TABLE IF NOT EXISTS system_metrics(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                at INTEGER NOT NULL,
                cpu_pct REAL,
                ram_pct REAL,
                disk_pct REAL,
                net_kbps REAL,
                process_count INTEGER
            );
            CREATE INDEX IF NOT EXISTS idx_metrics_at ON system_metrics(at);

            CREATE TABLE IF NOT EXISTS settings(
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            );

            CREATE TABLE IF NOT EXISTS patterns(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                kind TEXT NOT NULL,
                payload TEXT NOT NULL,
                detected_at INTEGER NOT NULL,
                confidence REAL NOT NULL
            );
        )SQL");
        set_version(1);
        AUR_INFO("Schema migrated to v1");
    }

    return true;
}

bool Database::wipe_user_data() {
    std::lock_guard lock(m_);
    return exec(R"SQL(
        DELETE FROM transitions;
        DELETE FROM sessions;
        DELETE FROM activity_events;
        DELETE FROM system_metrics;
        DELETE FROM applications;
        DELETE FROM patterns;
        VACUUM;
    )SQL");
}

std::uint64_t Database::file_size_bytes() const {
    std::error_code ec;
    if (!std::filesystem::exists(path_, ec)) return 0;
    return static_cast<std::uint64_t>(std::filesystem::file_size(path_, ec));
}

} // namespace aurevix