#pragma once

#include <exception>
#include <optional>
//

#include "../entity.h"
#include "../entity_helper.h"
#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "debug_name.h"
#include "has_subtype.h"
#include "is_item.h"

typedef int EntityID;

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
    CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item_id == -1; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    CanHoldItem& update(OptEntity item) {
        if (held_item_id != -1 && !(held_item()->cleanup) &&
            //
            (held_item()->has<IsItem>() &&
             !held_item()->get<IsItem>().is_held())
            //
        ) {
            log_warn(
                "you are updating the held item to null, but the old one isnt "
                "marked cleanup (and not being held) , this might be an issue "
                "if you are tring to "
                "delete it");
        }

        held_item_id = item ? item->id : -1;
        if (item) item->get<IsItem>().set_held_by(held_by);
        if (item && held_by == EntityType::Unknown) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item->id, item->get<DebugName>().name());
        }
        return *this;
    }

    // TODO this isnt const because we want to write to the item
    // we could make this const and then expose certain things that we want to
    // change separately like 'held_by'
    // (change to use update instead and make this const)
    [[nodiscard]] OptEntity held_item() {
        return EntityHelper::getEntityForID(held_item_id);
    }

    // const?
    [[nodiscard]] const OptEntity const_item() const {
        return EntityHelper::getEntityForID(held_item_id);
    }

    CanHoldItem& set_filter(EntityFilter ef) {
        filter = ef;
        return *this;
    }

    [[nodiscard]] bool can_hold(const Entity& item,
                                RespectFilter respect_filter) const {
        if (item.has<IsItem>()) {
            bool cbhb = item.get<IsItem>().can_be_held_by(held_by);
            // log_info("trying to pick up {} with {} and {} ",
            // item.get<DebugName>(), held_by, cbhb);
            if (!cbhb) return false;
        }
        if (!filter.matches(item, respect_filter)) return false;
        // By default accept anything
        return true;
    }

    CanHoldItem& update_held_by(EntityType hb) {
        held_by = hb;
        return *this;
    }

    [[nodiscard]] EntityFilter& get_filter() { return filter; }
    [[nodiscard]] const EntityFilter& get_filter() const { return filter; }

   private:
    EntityID held_item_id = -1;
    EntityType held_by;
    EntityFilter filter;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // we only need these for debug info
        s.value4b(held_item_id);
        s.object(filter);
    }
};
