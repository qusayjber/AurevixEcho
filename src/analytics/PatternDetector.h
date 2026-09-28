#pragma once
#include "core/TimeManager.h"
#include <string>
#include <vector>

namespace aurevix {

struct DetectedPattern {
    std::string kind;      // "repeat_workflow", "context_switching", "long_session", "startup_routine", "time_habit"
    std::string title;
    std::string detail;
    std::int64_t occurrences = 0;
    float        confidence  = 0.0f;  // 0..1
    std::int64_t sample_size = 0;
    Timestamp    window_from = 0;
    Timestamp    window_to   = 0;
};

class PatternDetector {
public:
    // Runs the local pattern algorithms on the given window.
    static std::vector<DetectedPattern> detect(Timestamp from, Timestamp to);
};

} // namespace aurevix