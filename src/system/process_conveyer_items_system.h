#pragma once

#include "../ah.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_hold_item.h"
#include "../components/conveys_held_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/transform.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "system_manager.h"

namespace system_manager {

// System for processing conveyer items during in-round updates
struct ProcessConveyerItemsSystem : public afterhours::System<> {
    virtual ~ProcessConveyerItemsSystem() = default;

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
    virtual void for_each_with(Entity& entity, float dt) override {
        if (entity
                .is_missing_any<CanHoldItem, ConveysHeldItem, CanBeTakenFrom>())
            return;

        const Transform& transform = entity.get<Transform>();

        CanHoldItem& canHold = entity.get<CanHoldItem>();
        CanBeTakenFrom& canBeTakenFrom = entity.get<CanBeTakenFrom>();
        ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

        // we are not holding anything
        if (canHold.empty()) return;

        // make sure no one can insta-grab from us
        canBeTakenFrom.update(false);

        // if the item is less than halfway, just keep moving it along
        // 0 is halfway btw
        if (conveysHeldItem.relative_item_pos <= 0.f) {
            conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
            return;
        }

        bool is_ipp = entity.has<IsPnumaticPipe>();

        const auto _conveyer_filter = [&entity,
                                       &canHold](const Entity& furn) -> bool {
            // cant be us
            if (entity.id == furn.id) return false;
            // needs to be able to hold something
            if (furn.is_missing<CanHoldItem>()) return false;
            const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
            // has to be empty
            if (furnCHI.is_holding_item()) return false;
            // can this furniture hold the item we are passing?
            // some have filters
            bool can_hold =
                furnCHI.can_hold(canHold.const_item(), RespectFilter::ReqOnly);

            return can_hold;
        };

        const auto _ipp_filter =
            [&entity, _conveyer_filter](const Entity& furn) -> bool {
            // if we are a pnumatic pipe, filter only down to our guy
            if (furn.is_missing<IsPnumaticPipe>()) return false;
            const IsPnumaticPipe& mypp = entity.get<IsPnumaticPipe>();
            if (mypp.paired_id != furn.id) return false;
            if (mypp.recieving) return false;
            return _conveyer_filter(furn);
        };

        OptEntity match;
        if (is_ipp) {
            auto pos = transform.as2();
            match = EQ().whereLambda(_ipp_filter)
                        .whereInRange(pos, MAX_SEARCH_RANGE)
                        .orderByDist(pos)
                        .gen_first();
        } else {
            match = EntityHelper::getMatchingEntityInFront(
                transform.as2(), 1.f, transform.face_direction(),
                _conveyer_filter);
        }

        // no match means we can't continue, stay in the middle
        if (!match) {
            conveysHeldItem.relative_item_pos = 0.f;
            canBeTakenFrom.update(true);
            return;
        }

        if (is_ipp) {
            entity.get<IsPnumaticPipe>().recieving = false;
        }

        // we got something that will take from us,
        // but only once we get close enough

        // so keep moving forward
        if (conveysHeldItem.relative_item_pos <= ConveysHeldItem::ITEM_END) {
            conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
            return;
        }

        // we reached the end, pass ownership

        CanHoldItem& ourCHI = entity.get<CanHoldItem>();

        CanHoldItem& matchCHI = match->get<CanHoldItem>();
        matchCHI.update(EntityHelper::getEntityAsSharedPtr(ourCHI.item()),
                        entity.id);

        ourCHI.update(nullptr, -1);

        canBeTakenFrom.update(
            true);  // we are ready to have someone grab from us
        // reset so that the next item we get starts from beginning
        conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

        if (match->has<CanBeTakenFrom>()) {
            match->get<CanBeTakenFrom>().update(false);
        }

        if (is_ipp && match->has<IsPnumaticPipe>()) {
            match->get<IsPnumaticPipe>().recieving = true;
        }

        if (match->is_missing<IsPnumaticPipe>() &&
            match->has<ConveysHeldItem>()) {
            // if we are pushing onto a conveyer, we need to make sure
            // we are keeping track of the orientations
            //
            //  --> --> in this case we want to place at 0.5f
            //
            //          ^
            //    -->-> |     in this we want to place at 0.f instead of -0.5
            bool send_to_middle = false;
            auto other_face = match->get<Transform>().face_direction();
            // TODO theres gotta be a math way to do this
            switch (transform.face_direction()) {
                case Transform::FORWARD:
                case Transform::BACK:
                    if (other_face == Transform::RIGHT ||
                        other_face == Transform::LEFT) {
                        send_to_middle = true;
                    }
                    break;
                case Transform::RIGHT:
                case Transform::LEFT:
                    if (other_face == Transform::FORWARD ||
                        other_face == Transform::BACK) {
                        send_to_middle = true;
                    }
                    break;
            }
            if (send_to_middle) {
                match->get<ConveysHeldItem>().relative_item_pos =
                    ConveysHeldItem::ITEM_START +
                    ((ConveysHeldItem::ITEM_END - ConveysHeldItem::ITEM_START) /
                     2.f);
            }
        }
    }
};

}  // namespace system_manager
