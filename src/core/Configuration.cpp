#include "core/Configuration.h"
#include <sstream>
#include <unordered_map>

namespace aurevix {

// A tiny serializer — deliberately not pulling in a JSON library.
static std::unordered_map<std::string,std::string> parse_kv(const std::string& s) {
    std::unordered_map<std::string,std::string> m;
    std::istringstream in(s);
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        m[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return m;
}

static int to_int(const std::string& s, int fallback) {
    try { return std::stoi(s); } catch (...) { return fallback; }
}
static bool to_bool(const std::string& s, bool fallback) {
    if (s == "1" || s == "true") return true;
    if (s == "0" || s == "false") return false;
    return fallback;
}
static float to_float(const std::string& s, float fallback) {
    try { return std::stof(s); } catch (...) { return fallback; }
}

void Configuration::load(const std::string& json) {
    auto m = parse_kv(json);
    auto get = [&](const char* k) -> std::string {
        auto it = m.find(k); return it == m.end() ? std::string{} : it->second;
    };
    first_run_completed = to_bool(get("first_run_completed"), first_run_completed);
    monitoring_enabled  = to_bool(get("monitoring_enabled"),  monitoring_enabled);
    startup_enabled     = to_bool(get("startup_enabled"),     startup_enabled);
    sample_interval_sec = to_int (get("sample_interval_sec"), sample_interval_sec);
    idle_threshold_sec  = to_int (get("idle_threshold_sec"),  idle_threshold_sec);
    monitor_processes   = to_bool(get("monitor_processes"),   monitor_processes);
    monitor_system      = to_bool(get("monitor_system"),      monitor_system);
    monitor_network     = to_bool(get("monitor_network"),     monitor_network);
    retention_days      = to_int (get("retention_days"),      retention_days);
    theme = static_cast<ThemeMode>(to_int(get("theme"), static_cast<int>(theme)));
    ui_scale            = to_float(get("ui_scale"),           ui_scale);
    reduced_motion      = to_bool(get("reduced_motion"),      reduced_motion);
}

std::string Configuration::save() const {
    std::ostringstream o;
    o << "first_run_completed=" << (first_run_completed ? 1 : 0) << "\n";
    o << "monitoring_enabled="  << (monitoring_enabled  ? 1 : 0) << "\n";
    o << "startup_enabled="     << (startup_enabled     ? 1 : 0) << "\n";
    o << "sample_interval_sec=" << sample_interval_sec            << "\n";
    o << "idle_threshold_sec="  << idle_threshold_sec             << "\n";
    o << "monitor_processes="   << (monitor_processes   ? 1 : 0) << "\n";
    o << "monitor_system="      << (monitor_system      ? 1 : 0) << "\n";
    o << "monitor_network="     << (monitor_network     ? 1 : 0) << "\n";
    o << "retention_days="      << retention_days                 << "\n";
    o << "theme="               << static_cast<int>(theme)        << "\n";
    o << "ui_scale="            << ui_scale                       << "\n";
    o << "reduced_motion="      << (reduced_motion ? 1 : 0)       << "\n";
    return o.str();
}

} // namespace aurevix