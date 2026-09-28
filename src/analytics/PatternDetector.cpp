#include "analytics/PatternDetector.h"
#include "analytics/SessionAnalyzer.h"
#include "storage/Repositories.h"
#include <algorithm>
#include <map>
#include <set>

namespace aurevix {

std::vector<DetectedPattern> PatternDetector::detect(Timestamp from, Timestamp to) {
    std::vector<DetectedPattern> out;

    auto transitions = TransitionRepository::between(from, to, 50000);
    auto counts = SessionAnalyzer::transition_counts(from, to, 200);

    // 1. Repeated workflow: most frequent single transition with count >= 5
    if (!counts.empty() && counts.front().count >= 5) {
        auto& top = counts.front();
        DetectedPattern p;
        p.kind = "repeat_workflow";
        p.title = "Repeated workflow";
        p.detail = top.from_name + " → " + top.to_name;
        p.occurrences = top.count;
        p.sample_size = static_cast<std::int64_t>(transitions.size());
        p.confidence = std::min(1.0f, float(top.count) / std::max<std::int64_t>(1, transitions.size()));
        p.window_from = from; p.window_to = to;
        out.push_back(std::move(p));
    }

    // 2. Frequent context switching
    {
        auto switches = SessionAnalyzer::switch_count(from, to);
        double hours = std::max(0.01, double(to - from) / 3600.0);
        double sw_per_hour = double(switches) / hours;
        if (switches >= 20 && sw_per_hour >= 8.0) {
            DetectedPattern p;
            p.kind = "context_switching";
            p.title = "Frequent context switching";
            p.detail = std::to_string(switches) + " switches over this window";
            p.occurrences = switches;
            p.sample_size = switches;
            p.confidence = std::min(1.0f, float(sw_per_hour / 60.0));
            p.window_from = from; p.window_to = to;
            out.push_back(std::move(p));
        }
    }

    // 3. Long sessions
    {
        auto longest = SessionAnalyzer::longest_session(from, to);
        if (longest && longest->duration_seconds >= 45 * 60) {
            DetectedPattern p;
            p.kind = "long_session";
            p.title = "Long session";
            p.detail = longest->app_name + " — " +
                       format_human_duration(longest->duration_seconds);
            p.occurrences = 1;
            p.sample_size = 1;
            p.confidence = std::min(1.0f, float(longest->duration_seconds) / 7200.0f);
            p.window_from = from; p.window_to = to;
            out.push_back(std::move(p));
        }
    }

    // 4. Repeated startup routine — find sequences that begin the day
    {
        // Group transitions by calendar day, take the first two transitions each day,
        // then find the most common bigram.
        std::map<std::string, int> morningBigram;
        std::map<std::string, std::set<std::string>> perDay;
        for (auto& t : transitions) {
            std::string day = format_day(t.at);
            if (perDay[day].size() < 2) {
                perDay[day].insert(t.from_name + "→" + t.to_name);
            }
        }
        for (auto& [day, sig] : perDay) {
            if (sig.size() >= 1) morningBigram[*sig.begin()]++;
        }
        int best = 0; std::string bestKey;
        for (auto& [k, v] : morningBigram) if (v > best) { best = v; bestKey = k; }
        if (best >= 3) {
            DetectedPattern p;
            p.kind = "startup_routine";
            p.title = "Repeated opening routine";
            p.detail = bestKey;
            p.occurrences = best;
            p.sample_size = static_cast<std::int64_t>(perDay.size());
            p.confidence = std::min(1.0f, float(best) / std::max(1, int(perDay.size())));
            p.window_from = from; p.window_to = to;
            out.push_back(std::move(p));
        }
    }

    // 5. Time-based habit — bins of transitions by hour
    {
        std::map<int, int> perHour;
        for (auto& t : transitions) {
            std::tm tm = local_tm(t.at);
            perHour[tm.tm_hour]++;
        }
        int total = 0, peakHour = -1, peakCount = 0;
        for (auto& [h, c] : perHour) { total += c; if (c > peakCount) { peakCount = c; peakHour = h; } }
        if (total >= 30 && peakHour >= 0 && peakCount >= total / 4) {
            DetectedPattern p;
            p.kind = "time_habit";
            p.title = "Time-based habit";
            char buf[128];
            std::snprintf(buf, sizeof(buf),
                "Activity concentrates around %02d:00–%02d:00", peakHour, peakHour + 1);
            p.detail = buf;
            p.occurrences = peakCount;
            p.sample_size = total;
            p.confidence = float(peakCount) / float(total);
            p.window_from = from; p.window_to = to;
            out.push_back(std::move(p));
        }
    }

    return out;
}

} // namespace aurevix