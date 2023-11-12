
#pragma once

#include <deque>

#include "../engine/pathfinder.h"
#include "../entity_helper.h"
#include "../vendor_include.h"
#include "base_component.h"

struct CanPathfind : public BaseComponent {
    virtual ~CanPathfind() {}

    [[nodiscard]] bool has_next_target() const { return !path.empty(); }
    [[nodiscard]] vec2 next_target() { return path[0]; }
    [[nodiscard]] bool is_path_empty() const { return !!path.empty(); }
    [[nodiscard]] vec2 get_local_target() { return local_target.value(); }

    void path_to(vec2 begin, vec2 end) {
        start = begin;
        goal = end;

        {
            auto new_path = bfs::find_path(
                start, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));

            update_path(new_path);

            log_info("gen path from {} to {} with {} steps", start, goal,
                     path.size());
        }

        // TODO For now we are just going to let the customer noclip
        if (path.empty()) {
            // TODO we dont know how to get the entity we are at the moment
            log_warn("Forcing {} {} to noclip in order to get valid path",
                     "some entity with canpathfind", "idk");
            // log_warn("Forcing {} {} to noclip in order to get valid path",
            // entity.get<DebugName>().name(), entity.id);
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

    void ensure_active_local_target(vec2 me) {
        if (local_target.has_value()) {
            // Only return if we have a target and we are still far away
            if (vec::distance(me, local_target.value()) > (TILESIZE / 2.f)) {
                return;
            }
        }

        local_target = next_target();
        path.pop_front();
    }

    [[nodiscard]] std::deque<vec2> get_path() const { return path; }

   private:
    std::optional<vec2> local_target;

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
