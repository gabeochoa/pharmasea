
#pragma once

#include <deque>
#include <thread>

template<typename T>
struct AtomicQueue {
    void push_back(const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        q.push_back(value);
    }

    void pop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        q.pop();
    }

    void pop_front() {
        std::lock_guard<std::mutex> lock(m_mutex);
        q.pop_front();
    }

    [[nodiscard]] T& front() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return q.front();
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
    std::deque<T> q;
    mutable std::mutex m_mutex;
};
