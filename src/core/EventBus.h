#pragma once
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <any>

namespace aurevix {

// A lightweight, thread-safe, synchronous event bus.
class EventBus {
public:
    using HandlerId = std::uint64_t;

    template <class E>
    HandlerId subscribe(std::function<void(const E&)> fn) {
        std::lock_guard lock(m_);
        auto id = ++next_id_;
        auto key = std::type_index(typeid(E));
        handlers_[key].push_back({id, [fn](const std::any& a) {
            fn(std::any_cast<const E&>(a));
        }});
        return id;
    }

    template <class E>
    void publish(const E& ev) {
        std::vector<std::pair<HandlerId, std::function<void(const std::any&)>>> copy;
        {
            std::lock_guard lock(m_);
            auto it = handlers_.find(std::type_index(typeid(E)));
            if (it == handlers_.end()) return;
            copy = it->second;
        }
        std::any boxed = ev;
        for (auto& [id, fn] : copy) fn(boxed);
    }

    void unsubscribe(HandlerId id) {
        std::lock_guard lock(m_);
        for (auto& [k, v] : handlers_) {
            v.erase(std::remove_if(v.begin(), v.end(),
                [id](auto& p){ return p.first == id; }), v.end());
        }
    }

private:
    std::mutex m_;
    std::unordered_map<std::type_index,
        std::vector<std::pair<HandlerId, std::function<void(const std::any&)>>>> handlers_;
    std::atomic<HandlerId> next_id_{0};
};

} // namespace aurevix