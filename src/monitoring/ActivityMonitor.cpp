#include "monitoring/ActivityMonitor.h"
#include "monitoring/WindowMonitor.h"
#include "storage/Repositories.h"
#include "core/TimeManager.h"
#include "core/Logger.h"

#include <set>

namespace aurevix {

ActivityMonitor::ActivityMonitor() = default;
ActivityMonitor::~ActivityMonitor() { stop(); }

void ActivityMonitor::start(const Configuration& cfg) {
    if (running_.load()) { update_configuration(cfg); return; }
    cfg_ = cfg;
    stop_.store(false);
    running_.store(true);
    day_start_ = start_of_day(now());
    worker_ = std::thread([this]{ worker_loop(); });
}

void ActivityMonitor::stop() {
    if (!running_.load()) return;
    stop_.store(true);
    if (worker_.joinable()) worker_.join();
    // flush current session as an ended event
    if (current_app_id_ != 0 && current_started_ != 0) {
        Timestamp end = now();
        if (end > current_started_ + 1) {
            ActivityRepository::insert_event(current_app_id_, current_started_, end);
            SessionRepository::insert(current_app_id_, current_started_, end);
        }
    }
    running_.store(false);
}

void ActivityMonitor::update_configuration(const Configuration& cfg) {
    std::lock_guard lock(snap_mutex_);
    cfg_ = cfg;
}

ActivitySnapshot ActivityMonitor::snapshot() {
    std::lock_guard lock(snap_mutex_);
    return snap_;
}

void ActivityMonitor::worker_loop() {
    AUR_INFO("Activity monitor worker started");
    while (!stop_.load()) {
        const int interval = std::max(1, cfg_.sample_interval_sec);
        for (int i = 0; i < interval * 10 && !stop_.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (stop_.load()) break;

        Timestamp t = now();

        // Reset daily counters at day boundary
        Timestamp today = start_of_day(t);
        if (today != day_start_) {
            day_start_ = today;
            apps_today_.clear();
            flow_.clear();
        }

        // Idle check
        std::uint32_t idle = WindowMonitor::seconds_since_last_input();
        bool idle_now = idle >= static_cast<std::uint32_t>(cfg_.idle_threshold_sec);

        auto fg = WindowMonitor::current_foreground();

        if (!idle_now && fg.valid) {
            std::int64_t app_id = ActivityRepository::ensure_application(fg.display_name);

            if (app_id != current_app_id_) {
                // Close previous session
                if (current_app_id_ != 0 && current_started_ != 0) {
                    Timestamp end = t;
                    if (end > current_started_ + 1) {
                        ActivityRepository::insert_event(current_app_id_, current_started_, end);
                        SessionRepository::insert(current_app_id_, current_started_, end);
                        ActivityRepository::touch_application(current_app_id_, end, end - current_started_);
                    }
                    TransitionRepository::insert(current_app_id_, app_id, t);
                    std::lock_guard lock(snap_mutex_);
                    snap_.switches_today++;
                }
                current_app_id_ = app_id;
                current_app_name_ = fg.display_name;
                current_started_ = t;

                std::lock_guard lock(snap_mutex_);
                apps_today_.insert(app_id);
                snap_.unique_apps_today = static_cast<std::int64_t>(apps_today_.size());

                // Flow: keep last 24 entries
                flow_.push_back(fg.display_name);
                if (flow_.size() > 24) flow_.erase(flow_.begin());
                snap_.recent_flow = flow_;
                snap_.current_app = fg.display_name;
                snap_.current_app_id = app_id;
                snap_.current_started_at = t;
            }
        } else if (idle_now && current_app_id_ != 0) {
            // Going idle — close session.
            Timestamp end = t;
            if (end > current_started_ + 1) {
                ActivityRepository::insert_event(current_app_id_, current_started_, end);
                SessionRepository::insert(current_app_id_, current_started_, end);
                ActivityRepository::touch_application(current_app_id_, end, end - current_started_);
                if (end - current_started_ >= 300) { // count focus sessions >= 5 min
                    std::lock_guard lock(snap_mutex_);
                    snap_.focus_sessions_today++;
                }
            }
            current_app_id_ = 0;
            current_app_name_.clear();
            current_started_ = 0;
            std::lock_guard lock(snap_mutex_);
            snap_.current_app.clear();
            snap_.current_app_id = 0;
            snap_.current_started_at = 0;
        }

        // Update snapshot counters
        {
            std::lock_guard lock(snap_mutex_);
            if (current_started_ != 0) {
                snap_.current_app = current_app_name_;
                snap_.current_app_id = current_app_id_;
                snap_.current_started_at = current_started_;
                snap_.current_seconds = t - current_started_;
            } else {
                snap_.current_seconds = 0;
            }
        }

        // System sampling (throttle to ~ every 5 s)
        if (cfg_.monitor_system && t - last_system_sample_ >= 5) {
            last_system_sample_ = t;
            SystemMetricRow m = sys_.sample();
            SystemMetricsRepository::insert(m);
        }
    }
    AUR_INFO("Activity monitor worker stopped");
}

} // namespace aurevix