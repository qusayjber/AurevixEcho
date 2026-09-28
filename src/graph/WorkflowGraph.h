#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace aurevix {

struct GraphNode {
    std::string  name;
    std::int64_t seconds = 0;
    std::int64_t visits = 0;
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float radius = 30.0f;
};

struct GraphEdge {
    int   from = 0;
    int   to = 0;
    std::int64_t count = 0;
};

class WorkflowGraph {
public:
    // Rebuild from transition data between given window.
    void build(std::int64_t from, std::int64_t to, int min_count = 1);

    void step_physics(float dt);           // simple force-directed relaxation
    void layout_circle();                  // initial layout

    const std::vector<GraphNode>& nodes() const { return nodes_; }
    const std::vector<GraphEdge>& edges() const { return edges_; }

    int index_of(const std::string& name) const;

private:
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    std::unordered_map<std::string, int> name_index_;
};

} // namespace aurevix