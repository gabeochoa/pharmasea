
#pragma once

// NOTE: Despite the name, this is a concurrent queue implementation.
// TODO(threading): Stop depending on Tracy's vendored copy and move this to a
// dedicated, project-owned third-party dependency (or our own impl).
//
// We use moodycamel::ConcurrentQueue for correctness: the previous
// mutex+deque wrapper returned references after unlocking, which was UB.

#include <atomic>
#include <cstddef>
#include <utility>

#include "../../vendor/tracy/client/tracy_concurrentqueue.h"

template<typename T>
struct AtomicQueue {
    void push_back(const T& value) {
        q.enqueue(value);
        m_size.fetch_add(1, std::memory_order_relaxed);
    }

    void push_back(T&& value) {
        q.enqueue(std::move(value));
        m_size.fetch_add(1, std::memory_order_relaxed);
    }

    // Pops an item if available. Returns false if empty.
    bool try_pop_front(T& out) {
        if (!q.try_dequeue(out)) return false;
        m_size.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    [[nodiscard]] bool empty() const {
        return size() == 0;
    }

    [[nodiscard]] size_t size() const {
        return m_size.load(std::memory_order_relaxed);
    }

   private:
    moodycamel::ConcurrentQueue<T> q;
    std::atomic<size_t> m_size{0};
};
