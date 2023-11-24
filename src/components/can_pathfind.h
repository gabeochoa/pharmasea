
#pragma once

#include <deque>

#include "../engine/pathfinder.h"
#include "../entity_helper.h"
#include "../vendor_include.h"
#include "base_component.h"

struct CanPathfind : public BaseComponent {
    virtual ~CanPathfind() {}

    [[nodiscard]] bool is_path_empty() const { return !!path.empty(); }
    [[nodiscard]] vec2 get_local_target() { return local_target.value(); }

    [[nodiscard]] bool travel_toward(vec2 end, float speed) {
        // Nothing to do we are already at the goal
        if (is_at_position(end)) return true;

        Transform& transform = parent->get<Transform>();
        vec2 me = transform.as2();

        // TODO always overwrite?
        global_target = end;

        if (is_path_empty()) {
            path_to(me, global_target.value());
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

   private:
    [[nodiscard]] bool is_at_position(vec2 position) {
        return vec::distance(parent->get<Transform>().as2(), position) <
               (TILESIZE / 2.f);
    }

    void move_transform_toward_local_target(float speed) {
        if (!local_target.has_value()) {
            log_warn("Tried to ensure local target but still dont have one");
            return;
        }

        Transform& transform = parent->get<Transform>();
        vec2 new_pos = transform.as2();
        vec2 tar = local_target.value();
        if (tar.x > transform.raw().x) new_pos.x += speed;
        if (tar.x < transform.raw().x) new_pos.x -= speed;

        if (tar.y > transform.raw().z) new_pos.y += speed;
        if (tar.y < transform.raw().z) new_pos.y -= speed;

        // TODO do we need to unr the whole person_update...() function with
        // collision?

        transform.update(vec::to3(new_pos));
    }

    void path_to(vec2 begin, vec2 end) {
        start = begin;
        goal = end;

        {
            auto new_path = bfs::find_path(
                start, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));

            update_path(new_path);

            log_trace("gen path from {} to {} with {} steps", start, goal,
                      path.size());
        }

        // TODO For now we are just going to let the customer noclip
        if (path.empty()) {
            // TODO we dont know how to get the entity we are at the moment
            log_warn("Forcing {} {} to noclip in order to get valid path",
                     "some entity with canpathfind", "idk");
            // log_warn("Forcing {} {} to noclip in order to get valid path",
            // entity.name(), entity.id);
            auto new_path =
                bfs::find_path(start, goal, [](auto&&) { return true; });
            update_path(new_path);
            // system_manager::logging_manager::announce(
            // entity, fmt::format("gen path from {} to {} with {} steps", me,
            // goal, p_size()));
        }
        // what happens if we get here and the path is still empty?
        if (path.empty()) {
            log_warn("no pathing even after noclip... {} {} {}=>{}", "my name",
                     "my id", start, goal);
        }
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

    vec2 start;
    vec2 goal;

    int path_size = 0;
    std::deque<vec2> path;

    void update_path(const std::deque<vec2>& new_path) {
        path = new_path;
        path_size = (int) path.size();
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(start);
        s.object(goal);

        s.value4b(path_size);
        s.container(path, path_size, [](S& sv, vec2 pos) { sv.object(pos); });
    }
};
