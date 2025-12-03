#pragma once

#include "../ah.h"
#include "../components/can_hold_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_rope_to_item.h"
#include "../components/is_item.h"
#include "../components/is_solid.h"
#include "../components/transform.h"
#include "../engine/pathfinder.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// System for processing has rope during in-round updates
struct ProcessHasRopeSystem : public afterhours::System<> {
    virtual ~ProcessHasRopeSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_nighttime();
        } catch (...) {
            return false;
        }
    }

    // TODO use system filters
    virtual void for_each_with(Entity& entity, float) override {
        if (entity.is_missing<CanHoldItem>()) return;
        if (entity.is_missing<HasRopeToItem>()) return;

        HasRopeToItem& hrti = entity.get<HasRopeToItem>();

        // No need to have rope if spout is put away
        const CanHoldItem& chi = entity.get<CanHoldItem>();
        if (chi.is_holding_item()) {
            hrti.clear();
            return;
        }

        // Find the player who is holding __OUR__ spout

        OptEntity player;
        for (const std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
            if (!e) continue;
            // only route to players
            if (!check_type(*e, EntityType::Player)) continue;
            const CanHoldItem& e_chi = e->get<CanHoldItem>();
            if (!e_chi.is_holding_item()) continue;
            const Item& i = e_chi.item();
            // that are holding spouts
            if (!check_type(i, EntityType::SodaSpout)) continue;
            // that match the one we were holding
            if (i.id != chi.last_id()) continue;
            player = *e;
        }
        if (!player) return;

        auto pos = player->get<Transform>().as2();

        // If we moved more then regenerate
        if (vec::distance(pos, hrti.goal()) > TILESIZE) {
            hrti.clear();
        }

        // Already generated
        if (hrti.was_generated()) return;

        auto new_path = pathfinder::find_path(entity.get<Transform>().as2(),
                                              pos, [](vec2) { return true; });

        std::vector<vec2> extended_path;
        std::optional<vec2> prev;
        for (auto p : new_path) {
            if (prev.has_value()) {
                extended_path.push_back(vec::lerp(prev.value(), p, 0.33f));
                extended_path.push_back(vec::lerp(prev.value(), p, 0.66f));
                extended_path.push_back(vec::lerp(prev.value(), p, 0.99f));
            }
            extended_path.push_back(p);
            prev = p;
        }

        for (auto p : extended_path) {
            Entity& item =
                EntityHelper::createItem(EntityType::SodaSpout, vec::to3(p));
            item.get<IsItem>().set_held_by(EntityType::Player, player->id);
            item.addComponent<IsSolid>();
            hrti.add(item);
        }
        hrti.mark_generated(pos);
    }
};

}  // namespace system_manager
