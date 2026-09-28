#pragma once
#include "core/TimeManager.h"
#include <string>
#include <vector>
#include <optional>

namespace aurevix {

struct ApplicationRow {
    std::int64_t id = 0;
    std::string  name;
    std::string  category;
    Timestamp    first_seen = 0;
    Timestamp    last_seen  = 0;
    std::int64_t total_seconds = 0;
};

struct SessionRow {
    std::int64_t id = 0;
    std::int64_t application_id = 0;
    std::string  app_name;
    Timestamp    started_at = 0;
    Timestamp    ended_at   = 0;
    std::int64_t duration_seconds = 0;
};

struct TransitionRow {
    std::int64_t id = 0;
    std::int64_t from_app = 0;
    std::int64_t to_app   = 0;
    std::string  from_name;
    std::string  to_name;
    Timestamp    at = 0;
};

struct SystemMetricRow {
    Timestamp    at = 0;
    double       cpu_pct = 0.0;
    double       ram_pct = 0.0;
    double       disk_pct = 0.0;
    double       net_kbps = 0.0;
    std::int64_t process_count = 0;
};

class ActivityRepository {
public:
    // Get or create the application entry, returns id.
    static std::int64_t ensure_application(const std::string& name);
    static std::optional<ApplicationRow> get_application(std::int64_t id);

    // Insert an activity event (a bounded active window [start,end] on one app).
    static void insert_event(std::int64_t app_id, Timestamp start, Timestamp end);

    // Add seconds to application total and update last_seen.
    static void touch_application(std::int64_t app_id, Timestamp at, std::int64_t add_seconds);
};

class SessionRepository {
public:
    static void insert(std::int64_t app_id, Timestamp start, Timestamp end);
    static std::vector<SessionRow> between(Timestamp from, Timestamp to, int limit = 5000);
    static std::vector<SessionRow> for_day(Timestamp day_start);
    static std::optional<SessionRow> longest_between(Timestamp from, Timestamp to);
};

class TransitionRepository {
public:
    static void insert(std::int64_t from_app, std::int64_t to_app, Timestamp at);
    static std::vector<TransitionRow> between(Timestamp from, Timestamp to, int limit = 20000);
};

class SystemMetricsRepository {
public:
    static void insert(const SystemMetricRow& m);
    static std::vector<SystemMetricRow> between(Timestamp from, Timestamp to, int limit = 10000);
    static void prune_before(Timestamp t);
};

class SettingsRepository {
public:
    static std::optional<std::string> get(const std::string& key);
    static void set(const std::string& key, const std::string& value);
    static void remove_old_data(Timestamp cutoff); // retention
};

} // namespace aurevix