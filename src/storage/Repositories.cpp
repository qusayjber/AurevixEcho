#include "storage/Repositories.h"
#include "storage/Database.h"
#include "core/Logger.h"

#include <mutex>

namespace aurevix {

static sqlite3* db() { return Database::instance().raw(); }

// ---------- ActivityRepository ----------
std::int64_t ActivityRepository::ensure_application(const std::string& name) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = db();
    Timestamp t = now();

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h, "SELECT id FROM applications WHERE name=?1;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    std::int64_t id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);

    if (id != 0) return id;

    sqlite3_prepare_v2(h,
        "INSERT INTO applications(name,first_seen,last_seen,total_seconds) VALUES(?1,?2,?2,0);",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, t);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(h);
}

std::optional<ApplicationRow> ActivityRepository::get_application(std::int64_t id) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "SELECT id,name,IFNULL(category,''),first_seen,last_seen,total_seconds "
        "FROM applications WHERE id=?1;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    std::optional<ApplicationRow> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ApplicationRow r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.name = (const char*)sqlite3_column_text(stmt, 1);
        r.category = (const char*)sqlite3_column_text(stmt, 2);
        r.first_seen = sqlite3_column_int64(stmt, 3);
        r.last_seen  = sqlite3_column_int64(stmt, 4);
        r.total_seconds = sqlite3_column_int64(stmt, 5);
        out = r;
    }
    sqlite3_finalize(stmt);
    return out;
}

void ActivityRepository::insert_event(std::int64_t app_id, Timestamp start, Timestamp end) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "INSERT INTO activity_events(application_id,started_at,ended_at,duration_seconds) "
        "VALUES(?1,?2,?3,?4);", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, app_id);
    sqlite3_bind_int64(stmt, 2, start);
    sqlite3_bind_int64(stmt, 3, end);
    sqlite3_bind_int64(stmt, 4, end > start ? end - start : 0);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void ActivityRepository::touch_application(std::int64_t app_id, Timestamp at, std::int64_t add_seconds) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "UPDATE applications SET last_seen=?1, total_seconds=total_seconds+?2 WHERE id=?3;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, at);
    sqlite3_bind_int64(stmt, 2, add_seconds);
    sqlite3_bind_int64(stmt, 3, app_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ---------- SessionRepository ----------
void SessionRepository::insert(std::int64_t app_id, Timestamp start, Timestamp end) {
    if (end <= start) return;
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "INSERT INTO sessions(application_id,started_at,ended_at,duration_seconds) "
        "VALUES(?1,?2,?3,?4);", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, app_id);
    sqlite3_bind_int64(stmt, 2, start);
    sqlite3_bind_int64(stmt, 3, end);
    sqlite3_bind_int64(stmt, 4, end - start);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<SessionRow> SessionRepository::between(Timestamp from, Timestamp to, int limit) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "SELECT s.id,s.application_id,IFNULL(a.name,''),s.started_at,s.ended_at,s.duration_seconds "
        "FROM sessions s LEFT JOIN applications a ON a.id=s.application_id "
        "WHERE s.started_at>=?1 AND s.started_at<=?2 ORDER BY s.started_at LIMIT ?3;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    sqlite3_bind_int(stmt, 3, limit);
    std::vector<SessionRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionRow r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.application_id = sqlite3_column_int64(stmt, 1);
        r.app_name = (const char*)sqlite3_column_text(stmt, 2);
        r.started_at = sqlite3_column_int64(stmt, 3);
        r.ended_at   = sqlite3_column_int64(stmt, 4);
        r.duration_seconds = sqlite3_column_int64(stmt, 5);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

std::vector<SessionRow> SessionRepository::for_day(Timestamp day_start) {
    return between(day_start, day_start + 86400);
}

std::optional<SessionRow> SessionRepository::longest_between(Timestamp from, Timestamp to) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "SELECT s.id,s.application_id,IFNULL(a.name,''),s.started_at,s.ended_at,s.duration_seconds "
        "FROM sessions s LEFT JOIN applications a ON a.id=s.application_id "
        "WHERE s.started_at>=?1 AND s.started_at<=?2 "
        "ORDER BY s.duration_seconds DESC LIMIT 1;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    std::optional<SessionRow> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SessionRow r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.application_id = sqlite3_column_int64(stmt, 1);
        r.app_name = (const char*)sqlite3_column_text(stmt, 2);
        r.started_at = sqlite3_column_int64(stmt, 3);
        r.ended_at   = sqlite3_column_int64(stmt, 4);
        r.duration_seconds = sqlite3_column_int64(stmt, 5);
        out = r;
    }
    sqlite3_finalize(stmt);
    return out;
}

// ---------- TransitionRepository ----------
void TransitionRepository::insert(std::int64_t from_app, std::int64_t to_app, Timestamp at) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "INSERT INTO transitions(from_app,to_app,at) VALUES(?1,?2,?3);",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from_app);
    sqlite3_bind_int64(stmt, 2, to_app);
    sqlite3_bind_int64(stmt, 3, at);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<TransitionRow> TransitionRepository::between(Timestamp from, Timestamp to, int limit) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "SELECT t.id,t.from_app,t.to_app,IFNULL(f.name,''),IFNULL(n.name,''),t.at "
        "FROM transitions t "
        "LEFT JOIN applications f ON f.id=t.from_app "
        "LEFT JOIN applications n ON n.id=t.to_app "
        "WHERE t.at>=?1 AND t.at<=?2 ORDER BY t.at LIMIT ?3;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    sqlite3_bind_int(stmt, 3, limit);
    std::vector<TransitionRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TransitionRow r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.from_app = sqlite3_column_int64(stmt, 1);
        r.to_app   = sqlite3_column_int64(stmt, 2);
        r.from_name = (const char*)sqlite3_column_text(stmt, 3);
        r.to_name   = (const char*)sqlite3_column_text(stmt, 4);
        r.at = sqlite3_column_int64(stmt, 5);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

// ---------- SystemMetricsRepository ----------
void SystemMetricsRepository::insert(const SystemMetricRow& m) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "INSERT INTO system_metrics(at,cpu_pct,ram_pct,disk_pct,net_kbps,process_count) "
        "VALUES(?1,?2,?3,?4,?5,?6);", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, m.at);
    sqlite3_bind_double(stmt, 2, m.cpu_pct);
    sqlite3_bind_double(stmt, 3, m.ram_pct);
    sqlite3_bind_double(stmt, 4, m.disk_pct);
    sqlite3_bind_double(stmt, 5, m.net_kbps);
    sqlite3_bind_int64(stmt, 6, m.process_count);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<SystemMetricRow> SystemMetricsRepository::between(Timestamp from, Timestamp to, int limit) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "SELECT at,cpu_pct,ram_pct,disk_pct,net_kbps,process_count "
        "FROM system_metrics WHERE at>=?1 AND at<=?2 ORDER BY at LIMIT ?3;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    sqlite3_bind_int(stmt, 3, limit);
    std::vector<SystemMetricRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SystemMetricRow r;
        r.at = sqlite3_column_int64(stmt, 0);
        r.cpu_pct = sqlite3_column_double(stmt, 1);
        r.ram_pct = sqlite3_column_double(stmt, 2);
        r.disk_pct = sqlite3_column_double(stmt, 3);
        r.net_kbps = sqlite3_column_double(stmt, 4);
        r.process_count = sqlite3_column_int64(stmt, 5);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

void SystemMetricsRepository::prune_before(Timestamp t) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(), "DELETE FROM system_metrics WHERE at < ?1;", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, t);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ---------- SettingsRepository ----------
std::optional<std::string> SettingsRepository::get(const std::string& key) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(), "SELECT value FROM settings WHERE key=?1;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<std::string> out;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        out = std::string((const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return out;
}

void SettingsRepository::set(const std::string& key, const std::string& value) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db(),
        "INSERT INTO settings(key,value) VALUES(?1,?2) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void SettingsRepository::remove_old_data(Timestamp cutoff) {
    std::lock_guard lock(Database::instance().mutex());
    const char* sqls[] = {
        "DELETE FROM activity_events WHERE started_at < ?1;",
        "DELETE FROM sessions       WHERE started_at < ?1;",
        "DELETE FROM transitions    WHERE at         < ?1;",
        "DELETE FROM system_metrics WHERE at         < ?1;",
        "DELETE FROM patterns       WHERE detected_at< ?1;",
    };
    for (auto* sql : sqls) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, cutoff);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}

} // namespace aurevix