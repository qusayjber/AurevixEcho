#include "graph/WorkflowGraph.h"
#include "analytics/SessionAnalyzer.h"
#include <cmath>
#include <algorithm>

namespace aurevix {

int WorkflowGraph::index_of(const std::string& name) const {
    auto it = name_index_.find(name);
    return it == name_index_.end() ? -1 : it->second;
}

void WorkflowGraph::build(std::int64_t from, std::int64_t to, int min_count) {
    nodes_.clear();
    edges_.clear();
    name_index_.clear();

    auto tops = SessionAnalyzer::top_apps(from, to, 24);
    for (auto& u : tops) {
        GraphNode n;
        n.name = u.app_name;
        n.seconds = u.seconds;
        n.visits = u.sessions;
        name_index_[n.name] = static_cast<int>(nodes_.size());
        nodes_.push_back(std::move(n));
    }

    auto tcs = SessionAnalyzer::transition_counts(from, to, 200);
    for (auto& tc : tcs) {
        if (tc.count < min_count) continue;
        int a = index_of(tc.from_name);
        int b = index_of(tc.to_name);
        if (a < 0 || b < 0 || a == b) continue;
        edges_.push_back({a, b, tc.count});
    }

    // Radius from usage
    std::int64_t max_secs = 1;
    for (auto& n : nodes_) if (n.seconds > max_secs) max_secs = n.seconds;
    for (auto& n : nodes_) {
        float frac = float(double(n.seconds) / double(max_secs));
        n.radius = 22.0f + 26.0f * std::sqrt(std::max(0.0f, frac));
    }

    layout_circle();
}

void WorkflowGraph::layout_circle() {
    const int n = static_cast<int>(nodes_.size());
    if (n == 0) return;
    const float R = 220.0f;
    const float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < n; ++i) {
        float a = float(i) / float(n) * 6.2831853f;
        nodes_[i].x = cx + R * std::cos(a);
        nodes_[i].y = cy + R * std::sin(a);
    }
}

void WorkflowGraph::step_physics(float dt) {
    const int n = static_cast<int>(nodes_.size());
    if (n < 2) return;

    // Repulsion between all nodes.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float dx = nodes_[j].x - nodes_[i].x;
            float dy = nodes_[j].y - nodes_[i].y;
            float d2 = dx*dx + dy*dy + 0.01f;
            float d  = std::sqrt(d2);
            float min_d = nodes_[i].radius + nodes_[j].radius + 24.0f;
            float force = (min_d * min_d) / d2;
            float fx = force * dx / d;
            float fy = force * dy / d;
            nodes_[i].vx -= fx * dt; nodes_[i].vy -= fy * dt;
            nodes_[j].vx += fx * dt; nodes_[j].vy += fy * dt;
        }
    }

    // Attraction along edges, weighted by count.
    std::int64_t max_count = 1;
    for (auto& e : edges_) if (e.count > max_count) max_count = e.count;
    for (auto& e : edges_) {
        auto& a = nodes_[e.from];
        auto& b = nodes_[e.to];
        float dx = b.x - a.x, dy = b.y - a.y;
        float d = std::sqrt(dx*dx + dy*dy) + 0.01f;
        float w = float(double(e.count) / double(max_count));
        float target = 160.0f;
        float k = (d - target) * (0.5f + w) * 0.02f;
        float fx = k * dx / d;
        float fy = k * dy / d;
        a.vx += fx * dt; a.vy += fy * dt;
        b.vx -= fx * dt; b.vy -= fy * dt;
    }

    // Center gravity + damping.
    for (auto& nd : nodes_) {
        nd.vx += -nd.x * 0.3f * dt;
        nd.vy += -nd.y * 0.3f * dt;
        nd.vx *= 0.90f;
        nd.vy *= 0.90f;
        nd.x += nd.vx * dt * 40.0f;
        nd.y += nd.vy * dt * 40.0f;
    }
}

} // namespace aurevix