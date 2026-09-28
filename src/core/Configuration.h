#pragma once
#include <string>
#include <cstdint>

namespace aurevix {

enum class ThemeMode { Dark, Light, System };

struct Configuration {
    // First run
    bool        first_run_completed = false;
    bool        monitoring_enabled  = true;
    bool        startup_enabled     = false;

    // Monitoring
    int         sample_interval_sec = 2;
    int         idle_threshold_sec  = 120;
    bool        monitor_processes   = true;
    bool        monitor_system      = true;
    bool        monitor_network     = true;

    // Privacy
    int         retention_days      = 30;

    // Appearance
    ThemeMode   theme               = ThemeMode::Dark;
    float       ui_scale            = 1.0f;
    bool        reduced_motion      = false;

    // Persistence
    void load(const std::string& json);
    std::string save() const;
};

} // namespace aurevix