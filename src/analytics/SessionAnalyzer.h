#pragma once
#include "storage/Repositories.h"
#include <vector>

namespace aurevix {

struct AppUsage {
    std::string app_name;
    std::int64_t seconds = 0;
    std::int64_t sessions = 0;
};

struct TransitionCount {
    std::string from_name;
    std::string to_name;
    std::int64_t count = 0;
};

class SessionAnalyzer {
public:
    // Ranked usage for a time window.
    static std::vector<AppUsage> top_apps(Timestamp from, Timestamp to, int topN = 10);

    // Transition counts between apps for a time window.
    static std::vector<TransitionCount> transition_counts(Timestamp from, Timestamp to, int topN = 30);

    // Longest single session in a window.
    static std::optional<SessionRow> longest_session(Timestamp from, Timestamp to);

    // Total active seconds in a window (sum of sessions).
    static std::int64_t total_active_seconds(Timestamp from, Timestamp to);

    // Number of app switches in a window.
    static std::int64_t switch_count(Timestamp from, Timestamp to);

    // Number of distinct apps used.
    static std::int64_t unique_app_count(Timestamp from, Timestamp to);

    // Compute behavior-genome metrics, 0..1 range each.
    struct Genome {
        float context_switching = 0.0f;
        float session_depth     = 0.0f;
        float app_diversity     = 0.0f;
        float repetition        = 0.0f;
        float idle_ratio        = 0.0f;
        float workflow_stability= 0.0f;
        std::int64_t samples = 0;
    };
    static Genome genome(Timestamp from, Timestamp to);
};

} // namespace aurevix