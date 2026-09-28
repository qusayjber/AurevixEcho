#pragma once
#include <cstdint>
#include <string>
#include <ctime>

namespace aurevix {

using Timestamp = std::int64_t; // Unix epoch, seconds

inline Timestamp now() {
    return static_cast<Timestamp>(std::time(nullptr));
}

inline std::tm local_tm(Timestamp t) {
    std::tm tm{};
    std::time_t tt = static_cast<std::time_t>(t);
    localtime_s(&tm, &tt);
    return tm;
}

inline std::string format_clock(Timestamp t) {
    std::tm tm = local_tm(t);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", tm.tm_hour, tm.tm_min);
    return buf;
}

inline std::string format_day(Timestamp t) {
    std::tm tm = local_tm(t);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

inline std::string format_human_duration(std::int64_t seconds) {
    if (seconds < 0) seconds = 0;
    std::int64_t h = seconds / 3600;
    std::int64_t m = (seconds % 3600) / 60;
    std::int64_t s = seconds % 60;
    char buf[48];
    if (h > 0)      std::snprintf(buf, sizeof(buf), "%lldh %02lldm", (long long)h, (long long)m);
    else if (m > 0) std::snprintf(buf, sizeof(buf), "%lldm %02llds", (long long)m, (long long)s);
    else            std::snprintf(buf, sizeof(buf), "%llds", (long long)s);
    return buf;
}

inline Timestamp start_of_day(Timestamp t) {
    std::tm tm = local_tm(t);
    tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    return static_cast<Timestamp>(std::mktime(&tm));
}

inline std::string greeting_for_now() {
    std::tm tm = local_tm(now());
    if (tm.tm_hour < 5)  return "Still up?";
    if (tm.tm_hour < 12) return "Good morning.";
    if (tm.tm_hour < 17) return "Good afternoon.";
    if (tm.tm_hour < 22) return "Good evening.";
    return "Good night.";
}

} // namespace aurevix