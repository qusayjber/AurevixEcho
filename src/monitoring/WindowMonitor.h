#pragma once
#include <string>
#include <cstdint>

namespace aurevix {

struct ForegroundInfo {
    std::string process_name;   // e.g. "chrome.exe"
    std::string display_name;   // e.g. "Google Chrome"
    std::uint32_t pid = 0;
    bool        valid = false;
};

class WindowMonitor {
public:
    // Returns info about the current foreground window. Safe to call frequently.
    static ForegroundInfo current_foreground();

    // Seconds since last user input (keyboard/mouse) — used for idle detection.
    static std::uint32_t seconds_since_last_input();
};

} // namespace aurevix