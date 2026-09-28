#pragma once
#include "core/Configuration.h"
#include "core/TimeManager.h"
#include "monitoring/SystemMonitor.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include <vector>
#include <set>
#include <cstdint>

namespace aurevix {

struct ActivitySnapshot {
    std::string current_app;          // display name
    std::int64_t current_app_id = 0;
    Timestamp   current_started_at = 0;
    std::int64_t current_seconds = 0;

    std::int64_t active_seconds_today = 0;
    std::int64_t switches_today = 0;
    std::int64_t focus_sessions_today = 0;
    std::int64_t unique_apps_today = 0;

    // Recent flow (most recent last) of display names.
    std::vector<std::string> recent_flow;
};

class ActivityMonitor {
public:
    using SnapshotCallback = std::function<void()>;

    ActivityMonitor();
    ~ActivityMonitor();

    void start(const Configuration& cfg);
    void stop();
    void update_configuration(const Configuration& cfg);

    bool running() const { return running_.load(); }

    ActivitySnapshot snapshot();

private:
    void worker_loop();

    Configuration cfg_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};
    std::thread worker_;

    SystemMonitor sys_;
    Timestamp last_system_sample_ = 0;

    // Session bookkeeping
    std::int64_t current_app_id_ = 0;
    std::string  current_app_name_;
    Timestamp    current_started_ = 0;
    std::int64_t last_activity_tick_ = 0;

    // Snapshot state
    mutable std::mutex snap_mutex_;
    ActivitySnapshot snap_;
    Timestamp day_start_ = 0;

    // Track IDs used today for unique count
    std::set<std::int64_t> apps_today_;
    std::vector<std::string> flow_;
};

} // namespace aurevix