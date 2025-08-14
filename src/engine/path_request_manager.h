
#pragma once

#include <chrono>
#include <functional>
#include <vector>

#include "../entity.h"
#include "atomic_queue.h"
#include "singleton.h"

struct PathRequestManager {
    using OnCompleteFn = std::function<void(const std::deque<vec2>&)>;

    struct PathRequest {
        int entity_id;
        vec2 start;
        vec2 end;
        OnCompleteFn onComplete;
    };

    struct PathResponse {
        int entity_id;
        std::deque<vec2> path;
        OnCompleteFn onComplete;
    };

    std::vector<vec2> entities_storage_;
    std::mutex entities_mutex_;
    AtomicQueue<PathRequest> request_queue;

    static void enqueue_request(const PathRequest& request);

    static void process_responses(
        const std::vector<std::shared_ptr<Entity>>& entities);

    //////////////
    //////////////
    //////////////

    AtomicQueue<PathResponse> response_queue;
    bool running = false;

    bool is_walkable(const vec2& pos);
    std::deque<vec2> find_path(const PathRequest& request);

    static std::thread start();

    void run() {
        // should probably always be about / above whats in the game.h
        constexpr float desiredFrameRate = 240.0f;
        constexpr std::chrono::duration<float> fixedTimeStep(1.0f /
                                                             desiredFrameRate);

        auto previousTime = std::chrono::high_resolution_clock::now();
        auto currentTime = previousTime;
        while (running) {
            currentTime = std::chrono::high_resolution_clock::now();
            float duration =
                std::chrono::duration_cast<std::chrono::duration<float>>(
                    currentTime - previousTime)
                    .count();
            size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            currentTime - previousTime)
                            .count();

            if (ms < 3) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (duration <= fixedTimeStep.count()) {
                continue;
            }
            previousTime = currentTime;

            while (!request_queue.empty()) {
                const auto& request = request_queue.front();
                auto path = find_path(request);
                request_queue.pop_front();
                response_queue.push_back(
                    PathResponse{.entity_id = request.entity_id, .path = path});
            }
        }
    }
};
