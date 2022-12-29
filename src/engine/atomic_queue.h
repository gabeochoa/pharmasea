
#pragma once

#include <deque>
#include <thread>

template<typename T>
class AtomicQueue {
   public:
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

    T& front() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return q.front();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return q.empty();
    }

   private:
    std::deque<T> q;
    mutable std::mutex m_mutex;
};
