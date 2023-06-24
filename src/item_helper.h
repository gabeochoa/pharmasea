
#pragma once

#include <thread>

#include "external_include.h"
//
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "globals.h"

//
#include "item.h"

typedef std::vector<std::shared_ptr<Item>> Items;
static Items client_items_DO_NOT_USE;
static Items server_items_DO_NOT_USE;
static Items client_lobby_items_DO_NOT_USE;
static Items server_lobby_items_DO_NOT_USE;

// TODO do we really need two of these ( EntityHelper)
// they basically both just act on vector<shared_ptr>
struct ItemHelper {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

    static Items& get_items() {
        const auto in_lobby = GameState::get().is(game::State::Lobby);

        if (is_server()) {
            return in_lobby ? server_lobby_items_DO_NOT_USE
                            : server_items_DO_NOT_USE;
        }
        // Right now we only have server/client thread, but in the future if we
        // have more then we should check these

        // auto client_thread_id =
        // GLOBALS.get_or_default("client_thread_id", std::thread::id());
        return in_lobby ? client_lobby_items_DO_NOT_USE
                        : client_items_DO_NOT_USE;
    }

    static void addItem(std::shared_ptr<Item> e) {
        get_items().push_back(e);
        // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        // Note: addShape merges shapes next to each other
        //      this reduces the amount of loops overall

        // nav->addShape(getPolyForItem(e));
        // nav->addItem(e->id, getPolyForItem(e));
        // cache_is_walkable.clear();
    }

    static void removeItem(std::shared_ptr<Item> e) {
        // if (e->add_to_navmesh()) {
        // auto nav = GLOBALS.get_ptr<NavMesh>("navmesh");
        // nav->removeItem(e->id);
        // cache_is_walkable.clear();
        // }

        auto items = get_items();

        auto it = items.begin();
        while (it != get_items().end()) {
            if ((*it)->id == e->id) {
                items.erase(it);
                continue;
            }
            it++;
        }
    }

    static void cleanup() {
        // Cleanup items marked cleanup
        auto items = get_items();

        auto it = items.begin();
        while (it != items.end()) {
            if ((*it)->cleanup) {
                items.erase(it);
                continue;
            }
            it++;
        }
    }

    enum ForEachFlow {
        NormalFlow = 0,
        Continue = 1,
        Break = 2,
    };

    static void forEachItem(
        std::function<ForEachFlow(std::shared_ptr<Item>)> cb) {
        for (auto e : get_items()) {
            if (!e) continue;
            auto fef = cb(e);
            if (fef == 1) continue;
            if (fef == 2) break;
        }
    }

    template<typename T>
    static constexpr std::vector<std::shared_ptr<T>> getitemsInRange(
        vec2 pos, float range) {
        std::vector<std::shared_ptr<T>> matching;
        for (auto& e : get_items()) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (vec::distance(pos, vec::to2(e->position)) < range) {
                matching.push_back(s);
            }
        }
        return matching;
    }

    template<typename T>
    static constexpr std::shared_ptr<T> getClosestMatchingItem(
        vec2 pos, float range,
        std::function<bool(std::shared_ptr<T>)> filter = {}) {
        float best_distance = range;
        std::shared_ptr<T> best_so_far;
        for (auto& e : get_items()) {
            auto s = dynamic_pointer_cast<T>(e);
            if (!s) continue;
            if (filter && !filter(s)) continue;
            float d = vec::distance(pos, vec::to2(e->position));
            if (d > range) continue;
            if (d < best_distance) {
                best_so_far = s;
                best_distance = d;
            }
        }
        return best_so_far;
    }

#pragma clang diagnostic pop
};
