#include "analytics/SessionAnalyzer.h"
#include "storage/Database.h"
#include "core/Logger.h"
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>

namespace aurevix {

std::vector<AppUsage> SessionAnalyzer::top_apps(Timestamp from, Timestamp to, int topN) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = Database::instance().raw();
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h,
        "SELECT IFNULL(a.name,''), SUM(s.duration_seconds), COUNT(s.id) "
        "FROM sessions s LEFT JOIN applications a ON a.id=s.application_id "
        "WHERE s.started_at>=?1 AND s.started_at<=?2 "
        "GROUP BY s.application_id ORDER BY SUM(s.duration_seconds) DESC LIMIT ?3;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    sqlite3_bind_int(stmt, 3, topN);
    std::vector<AppUsage> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AppUsage u;
        u.app_name = (const char*)sqlite3_column_text(stmt, 0);
        u.seconds  = sqlite3_column_int64(stmt, 1);
        u.sessions = sqlite3_column_int64(stmt, 2);
        rows.push_back(std::move(u));
    }
    sqlite3_finalize(stmt);
    return rows;
}

std::vector<TransitionCount> SessionAnalyzer::transition_counts(Timestamp from, Timestamp to, int topN) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = Database::instance().raw();
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h,
        "SELECT IFNULL(f.name,''), IFNULL(n.name,''), COUNT(*) "
        "FROM transitions t "
        "LEFT JOIN applications f ON f.id=t.from_app "
        "LEFT JOIN applications n ON n.id=t.to_app "
        "WHERE t.at>=?1 AND t.at<=?2 "
        "GROUP BY t.from_app, t.to_app ORDER BY COUNT(*) DESC LIMIT ?3;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    sqlite3_bind_int(stmt, 3, topN);
    std::vector<TransitionCount> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TransitionCount t;
        t.from_name = (const char*)sqlite3_column_text(stmt, 0);
        t.to_name   = (const char*)sqlite3_column_text(stmt, 1);
        t.count     = sqlite3_column_int64(stmt, 2);
        if (!t.from_name.empty() && !t.to_name.empty()) rows.push_back(std::move(t));
    }
    sqlite3_finalize(stmt);
    return rows;
}

std::optional<SessionRow> SessionAnalyzer::longest_session(Timestamp from, Timestamp to) {
    return SessionRepository::longest_between(from, to);
}

std::int64_t SessionAnalyzer::total_active_seconds(Timestamp from, Timestamp to) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = Database::instance().raw();
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h,
        "SELECT IFNULL(SUM(duration_seconds),0) FROM sessions WHERE started_at>=?1 AND started_at<=?2;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    std::int64_t total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) total = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return total;
}

std::int64_t SessionAnalyzer::switch_count(Timestamp from, Timestamp to) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = Database::instance().raw();
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h, "SELECT COUNT(*) FROM transitions WHERE at>=?1 AND at<=?2;",
                       -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    std::int64_t n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

std::int64_t SessionAnalyzer::unique_app_count(Timestamp from, Timestamp to) {
    std::lock_guard lock(Database::instance().mutex());
    sqlite3* h = Database::instance().raw();
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(h,
        "SELECT COUNT(DISTINCT application_id) FROM sessions WHERE started_at>=?1 AND started_at<=?2;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    std::int64_t n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

SessionAnalyzer::Genome SessionAnalyzer::genome(Timestamp from, Timestamp to) {
    Genome g;
    auto sessions = SessionRepository::between(from, to, 100000);
    if (sessions.empty()) return g;

    g.samples = static_cast<std::int64_t>(sessions.size());

    // Context switching: switches per active hour.
    auto total_active = 0LL;
    for (auto& s : sessions) total_active += s.duration_seconds;
    auto switches = switch_count(from, to);
    double hours = std::max(0.01, double(total_active) / 3600.0);
    double sw_per_hour = double(switches) / hours;
    // Normalize: 0..60 switches/hour mapped to 0..1
    g.context_switching = std::min(1.0f, float(sw_per_hour / 60.0));

    // Session depth: average session duration mapped 0..1 (30 min = 1.0).
    double avg_secs = double(total_active) / double(sessions.size());
    g.session_depth = std::min(1.0f, float(avg_secs / 1800.0));

    // App diversity: unique apps mapped 0..20.
    auto uniq = unique_app_count(from, to);
    g.app_diversity = std::min(1.0f, float(double(uniq) / 20.0));

    // Repetition: fraction of transitions that are the same edge repeated.
    auto tc = transition_counts(from, to, 1000);
    std::int64_t total_t = 0, top_t = 0;
    for (auto& c : tc) { total_t += c.count; if (c.count > top_t) top_t = c.count; }
    if (total_t > 0) g.repetition = std::min(1.0f, float(double(top_t) / double(total_t)) * 4.0f);

    // Idle ratio: complement of active fraction within observed day span.
    double span = double(to - from);
    if (span > 0) {
        double active_ratio = double(total_active) / span;
        g.idle_ratio = std::clamp(1.0f - float(active_ratio) * 3.0f, 0.0f, 1.0f);
    }

    // Workflow stability: 1 - (unique_bigrams / total_bigrams)
    std::map<std::pair<std::string,std::string>, int> bigrams;
    std::int64_t bcount = 0;
    for (auto& c : tc) { bigrams[{c.from_name, c.to_name}] += int(c.count); bcount += c.count; }
    if (bcount > 0)
        g.workflow_stability = std::clamp(1.0f - float(bigrams.size()) / float(bcount) * 8.0f, 0.0f, 1.0f);
    else
        g.workflow_stability = 0.0f;

    return g;
}

} // namespace aurevix