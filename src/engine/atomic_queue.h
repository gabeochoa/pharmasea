
#pragma once

// NOTE: Despite the name, this is a thread-safe queue implementation.
//
// This is a minimal mutex-based queue with a safe API (no reference returned
// after unlocking). It is intended to prevent UB/races in producer/consumer
// paths without pulling in a heavier dependency.
//
// TODO(threading): Consider replacing with a dedicated MPMC queue (e.g.
// moodycamel::ConcurrentQueue) if/when we want lock-free behavior. Tracy's
// vendored concurrentqueue differs from upstream and isn't a drop-in.

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

template<typename T>
struct AtomicQueue {
    void push_back(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        q.push_back(value);
    }

    void push_back(T&& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        q.push_back(std::move(value));
    }

    // Pops an item if available. Returns false if empty.
    bool try_pop_front(T& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (q.empty()) return false;
        out = std::move(q.front());
        q.pop_front();
        return true;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return q.empty();
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return q.size();
    }

   private:
    mutable std::mutex m_mutex;
    std::deque<T> q;
};
