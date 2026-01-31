
#pragma once

#include <random>

#include "../../ah.h"
#include "../../components/is_floor_marker.h"
#include "../../components/is_progression_manager.h"
#include "../../components/is_round_settings_manager.h"
#include "../../libraries/config_key_library.h"
#include "../../engine/log.h"
#include "../../engine/random_engine.h"
#include "../../entity_query.h"
#include "../../entity_type.h"

namespace system_manager {

namespace store {

inline void cleanup_old_store_options() {
    OptEntity cart_area =
        EntityQuery()
            .whereHasComponent<IsFloorMarker>()
            .whereLambda([](const Entity& entity) {
                if (entity.is_missing<IsFloorMarker>()) return false;
                const IsFloorMarker& fm = entity.get<IsFloorMarker>();
                return fm.type == IsFloorMarker::Type::Store_PurchaseArea;
            })
            .include_store_entities()
            .gen_first();

    OptEntity locked_area =
        EntityQuery()
            .whereHasComponent<IsFloorMarker>()
            .whereLambda([](const Entity& entity) {
                if (entity.is_missing<IsFloorMarker>()) return false;
                const IsFloorMarker& fm = entity.get<IsFloorMarker>();
                return fm.type == IsFloorMarker::Type::Store_LockedArea;
            })
            .include_store_entities()
            .gen_first();

    for (Entity& entity : EntityQuery()
                              .whereHasComponent<IsStoreSpawned>()
                              .include_store_entities()
                              .gen()) {
        // ignore antyhing in the cart
        if (cart_area) {
            if (cart_area->get<IsFloorMarker>().is_marked(entity.id)) {
                continue;
            }
        }

        // ignore anything locked
        if (locked_area) {
            if (locked_area->get<IsFloorMarker>().is_marked(entity.id)) {
                continue;
            }
        }

        entity.cleanup = true;

        // Also cleanup the item its holding if it has one
        if (entity.is_missing<CanHoldItem>()) continue;
        CanHoldItem& chi = entity.get<CanHoldItem>();
        if (!chi.is_holding_item()) continue;
        OptEntity held_opt = chi.item();
        if (held_opt) {
            held_opt.asE().cleanup = true;
        } else {
            chi.update(nullptr, entity.id);
        }
    }
}

inline void generate_store_options() {
    // Figure out what kinds of things we can spawn generally
    // - what is spawnable?
    // - are they capped by progression? (alcohol / fruits for sure
    // right?) choose a couple options to spawn
    // - how many?
    // spawn them
    // - use the place machine thing

    OptEntity spawn_area =
        EQ().whereFloorMarkerOfType(IsFloorMarker::Type::Store_SpawnArea)
            .gen_first();

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsProgressionManager& ipp = sophie.get<IsProgressionManager>();
    const EntityTypeSet& unlocked = ipp.enabled_entity_types();
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    int num_to_spawn = irsm.get<int>(ConfigKey::NumStoreSpawns);

    // NOTE: areas expand outward so as2() refers to the center
    // so we have to go back half the size
    Transform& area_transform = spawn_area->get<Transform>();
    vec2 area_origin = area_transform.as2();
    float half_width = area_transform.sizex() / 2.f;
    float half_height = area_transform.sizez() / 2.f;
    float reset_x = area_origin.x - half_width;
    float reset_y = area_origin.y - half_height;

    vec2 spawn_position = vec2{reset_x, reset_y};

    while (num_to_spawn) {
        int entity_type_id =
            bitset_utils::get_random_enabled_bit(unlocked, RandomEngine::rng());
        EntityType etype = magic_enum::enum_value<EntityType>(entity_type_id);
        if (get_price_for_entity_type(etype) <= 0) continue;

        log_info("generate_store_options: random: {}",
                 magic_enum::enum_name<EntityType>(etype));

        auto& entity = EntityHelper::createEntity();
        entity.addComponent<IsStoreSpawned>();
        bool success = convert_to_type(etype, entity, spawn_position);
        if (success) {
            num_to_spawn--;
        } else {
            entity.cleanup = true;
        }

        spawn_position.x += 2;

        if (spawn_position.x > (area_origin.x + half_width)) {
            spawn_position.x = reset_x;
            spawn_position.y += 2;
        } else if (spawn_position.y > (area_origin.y + half_height)) {
            reset_x += 1;
            spawn_position.x = reset_x;
            spawn_position.y = reset_y;
        }
    }
}

}  // namespace store
}  // namespace system_manager