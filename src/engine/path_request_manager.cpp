
#include "path_request_manager.h"

#include "../components/can_pathfind.h"
#include "../entity.h"
#include "../system/input_process_manager.h"
#include "bfs.h"

static std::shared_ptr<PathRequestManager> g_path_request_manager;

void PathRequestManager::process_responses(
    const std::vector<std::shared_ptr<Entity>>& entities) {
    //
    {
        std::lock_guard<std::mutex> lock(
            g_path_request_manager->entities_mutex_);
        g_path_request_manager->entities_storage_ = entities;
    }

    while (!g_path_request_manager->response_queue.empty()) {
        const auto& response = g_path_request_manager->response_queue.front();

        OptEntity requester = EntityHelper::getEntityForID(response.entity_id);
        if (requester.has_value()) {
            requester->get<CanPathfind>().update_path(response.path);
        } else {
            log_warn("Path requester {} no longer exists", response.entity_id);
        }
        g_path_request_manager->response_queue.pop_front();
    }
}

void PathRequestManager::enqueue_request(const PathRequest& request) {
    if (!g_path_request_manager) {
        log_error(
            "Requesting path for entity {} but you dont have a running path "
            "manager thread",
            request.entity_id);
    }
    g_path_request_manager->request_queue.push_back(request);
}

std::thread PathRequestManager::start() {
    g_path_request_manager.reset(new PathRequestManager());
    g_path_request_manager->running = true;
    return std::thread(
        std::bind(&PathRequestManager::run, g_path_request_manager.get()));
}

bool PathRequestManager::is_walkable(const vec2& pos) {
    bool hit_impassable = false;
    for (const auto& e : entities_storage_) {
        if (!e) continue;
        Entity& entity = *e;
        if (entity.is_missing<Transform>()) continue;

        // Ignore non colliable objects
        if (!system_manager::input_process_manager::is_collidable(entity))
            continue;
        // ignore things that are not at this location
        if (vec::distance(entity.template get<Transform>().as2(), pos) >
            TILESIZE / 2.f)
            continue;

        // is_collidable and inside this square
        hit_impassable = true;
        break;
    }
    return !hit_impassable;
}

std::deque<vec2> PathRequestManager::find_path(const PathRequest& request) {
    return bfs::find_path(request.start, request.end, [&](const vec2& pos) {
        return this->is_walkable(pos);
    });
}
