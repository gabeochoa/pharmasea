#pragma once

#include "../../../ah.h"
#include "../../../components/can_be_taken_from.h"
#include "../../../components/can_grab_from_other_furniture.h"
#include "../../../components/can_hold_item.h"
#include "../../../components/conveys_held_item.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/transform.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../../entity_query.h"
#include "../../core/system_manager.h"

namespace system_manager {

// System for processing grabber items during in-round updates
struct ProcessGrabberItemsSystem
    : public afterhours::System<Transform, CanHoldItem, ConveysHeldItem,
                                CanGrabFromOtherFurniture> {
    virtual ~ProcessGrabberItemsSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               CanHoldItem& canHold,
                               ConveysHeldItem& conveysHeldItem,
                               CanGrabFromOtherFurniture&, float) override {
        // we are already holding something so
        if (canHold.is_holding_item()) return;

        auto behind =
            transform.offsetFaceDirection(transform.face_direction(), 180);
        auto match = EQ().getMatchingEntityInFront(
            transform.as2(), behind, 1.f, [&entity](const Entity& furn) {
                // cant be us
                if (entity.id == furn.id) return false;
                // needs to be able to hold something
                if (furn.is_missing<CanHoldItem>()) return false;
                const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
                // doesnt have anything
                if (furnCHI.empty()) return false;

                // Can we hold the item it has?
                OptEntity held_opt = furnCHI.const_item();
                if (!held_opt) return false;
                bool can_hold = entity.get<CanHoldItem>().can_hold(
                    held_opt.asE(), RespectFilter::All);

                // we cant
                if (!can_hold) return false;

                // We only check CanBe when it exists because everyone else can
                // always be taken from with a grabber
                if (furn.is_missing<CanBeTakenFrom>()) return true;
                return furn.get<CanBeTakenFrom>().can_take_from();
            });

        // No furniture behind us
        if (!match) return;

        // Grab from the furniture match
        CanHoldItem& matchCHI = match->get<CanHoldItem>();
        CanHoldItem& ourCHI = entity.get<CanHoldItem>();

        OptEntity grabbed_opt = matchCHI.item();
        if (!grabbed_opt) {
            matchCHI.update(nullptr, entity_id::INVALID);
            return;
        }
        ourCHI.update(grabbed_opt.asE(), entity.id);
        matchCHI.update(nullptr, entity_id::INVALID);

        conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
    }
};

}  // namespace system_manager