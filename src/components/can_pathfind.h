
#pragma once

#include <deque>

#include "../engine/path_request_manager.h"
#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_ref.h"
#include "../vendor_include.h"
#include "base_component.h"

struct CanPathfind : public BaseComponent {
    [[nodiscard]] bool is_path_empty() const { return !!path.empty(); }
    [[nodiscard]] vec2 get_local_target() { return local_target.value(); }

    [[nodiscard]] bool travel_toward(vec2 end, float speed) {
        Entity& parent = EntityHelper::getEnforcedEntityForID(parent.id);

        // Nothing to do we are already at the goal
        if (is_at_position(end)) return true;

        // Waiting for our path request to be resolved
        if (has_active_request) {
            return false;
        }

        Transform& transform = parent.get<Transform>();
        vec2 me = transform.as2();

        global_target = end;

        if (is_path_empty()) {
            path_to(me, global_target.value());
            return false;
        }
        ensure_active_local_target();
        move_transform_toward_local_target(speed);

        if (local_target.has_value())
            transform.turn_to_face_pos(local_target.value());

        return is_at_position(end);
    }

    [[nodiscard]] std::deque<vec2> get_path() const { return path; }

    void for_each_path_location(const std::function<void(vec2)>& cb) const {
        if (is_path_empty()) return;
        for (auto location : path) {
            cb(location);
        }
    }

    [[nodiscard]] size_t get_max_length() const { return max_path_length; }

    void update_path(const std::deque<vec2>& new_path) {
        path = new_path;
        path_size = (int) path.size();

        has_active_request = false;
        max_path_length = std::max(max_path_length, path.size());
        log_trace("{} recieved a path of length {}", parent.id, path.size());
    }

    auto& set_parent(EntityID id) {
        parent.set_id(id);
        return *this;
    }

   private:
    [[nodiscard]] bool is_at_position(vec2 position) {
        const Entity& owner = EntityHelper::getEnforcedEntityForID(parent.id);
        return vec::distance(owner.get<Transform>().as2(), position) <
               (TILESIZE / 2.f);
    }

    void move_transform_toward_local_target(float speed) {
        Entity& owner = EntityHelper::getEnforcedEntityForID(parent.id);
        if (!local_target.has_value()) {
            log_warn("Tried to ensure local target but still dont have one");
            return;
        }

        Transform& transform = owner.get<Transform>();
        vec2 new_pos = transform.as2();
        vec2 tar = local_target.value();
        if (tar.x > transform.raw().x) new_pos.x += speed;
        if (tar.x < transform.raw().x) new_pos.x -= speed;

        if (tar.y > transform.raw().z) new_pos.y += speed;
        if (tar.y < transform.raw().z) new_pos.y -= speed;

        // TODO do we need to unr the whole person_update...() function with
        // collision?
        // // TODO what does "unr" mean ?

        transform.update(vec::to3(new_pos));
    }

    void path_to(vec2 begin, vec2 end) {
        if (has_active_request) {
            // just keep waiting
            return;
        }

        if (!path.empty()) {
            return;
        }

        start = begin;
        goal = end;

        PathRequestManager::enqueue_request(PathRequestManager::PathRequest{
            .entity_id = parent.id, .start = start, .end = goal});

        has_active_request = true;
        log_trace("{} requested path from {} to {} ", parent.id, start, goal);
    }

    void ensure_active_local_target() {
        if (local_target.has_value()) {
            // Only return if we have a target and we are still far away
            if (!is_at_position(local_target.value())) return;
        }

        local_target = path[0];
        path.pop_front();
    }

    std::optional<vec2> local_target;
    std::optional<vec2> global_target;

    bool has_active_request = false;
    vec2 start;
    vec2 goal;

    int path_size = 0;
    std::deque<vec2> path;
    size_t max_path_length = 0;

    EntityRef parent{};

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(parent);

        s.object(start);
        s.object(goal);

        s.value4b(path_size);
        s.container(path, path_size, [](S& sv, vec2 pos) { sv.object(pos); });
    }
};
