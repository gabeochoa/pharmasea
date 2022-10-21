#include <iostream>
#include <chrono>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <memory>
#include <utility>

/*
auto timer1 = new auto(
    make_interval_timer([] {
        std::cout << "tick" <<std::endl;
    },
    std::chrono::milliseconds{ 500 }, start_now, 5)
);
*/

using interval_t = std::chrono::milliseconds;
constexpr struct start_now_t {} start_now;

template <typename Function>
class interval_timer {
    class impl {
        Function f;
        const interval_t interval;
        int max_iter = -1;
        std::thread thread;
        std::mutex mtx;
        std::condition_variable cvar;
        bool enabled = false;

        void timer() {
            auto deadline = std::chrono::steady_clock::now() + interval;
            std::unique_lock<std::mutex> lock{ mtx };
            while (enabled && max_iter > 0) {
                if (cvar.wait_until(lock, deadline) == std::cv_status::timeout) {
                    lock.unlock();
                    f();
                    if (max_iter > 0) {
                        --max_iter;
                    }
                    deadline += interval;
                    lock.lock();
                }
            }
        }

    public:
        impl(Function f, interval_t interval, int max_iter_in) :
            f(std::move(f)), interval(std::move(interval)), max_iter(std::move(max_iter_in)) {}

        ~impl() {
            stop();
        }

        void start() {
            if (!enabled) {
                enabled = true;
                thread = std::thread(&impl::timer, this);
            }
        }

        void stop() {
            if (enabled) {
                {
                    std::lock_guard<std::mutex> _{ mtx };
                    enabled = false;
                }
                cvar.notify_one();
                thread.join();
            }
        }
    };

    std::unique_ptr<impl> pimpl;

public:
    interval_timer(Function f, interval_t interval, int max_iter = -1) :
        pimpl{ new impl(std::move(f), std::move(interval), std::move(max_iter)) } {}

    interval_timer(Function f, interval_t interval, start_now_t, int max_iter = -1) :
        interval_timer(std::move(f), std::move(interval), std::move(max_iter)) {
        start();
    }

    void start() { pimpl->start(); }
    void stop() { pimpl->stop(); }
};

template <typename Function, typename... Args>
interval_timer<typename std::decay<Function>::type>
make_interval_timer(Function&& f, Args&&... args) {
    return { std::forward<Function>(f), std::forward<Args>(args)... };
}
