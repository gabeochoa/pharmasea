#pragma once

#include <exception>
#include <optional>
//

#include "../entity.h"
#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "debug_name.h"
#include "has_subtype.h"
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(IsItem::HeldBy::UNKNOWN) {}
    CanHoldItem(IsItem::HeldBy hb) : held_by(hb) {}

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_item);
    }

    CanHoldItem& update(std::shared_ptr<Entity> item) {
        if (held_item != nullptr && !held_item->cleanup &&
            //
            (held_item->has<IsItem>() && !held_item->get<IsItem>().is_held())
            //
        ) {
            log_warn(
                "you are updating the held item to null, but the old one isnt "
                "marked cleanup (and not being held) , this might be an issue "
                "if you are tring to "
                "delete it");
        }

        held_item = item;
        if (held_item) held_item->get<IsItem>().set_held_by(held_by);
        if (held_item && held_by == IsItem::HeldBy::UNKNOWN) {
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
    [[nodiscard]] std::shared_ptr<Entity>& item() { return held_item; }

    // const?
    [[nodiscard]] const std::shared_ptr<Entity> const_item() const {
        return held_item;
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

    CanHoldItem& update_held_by(IsItem::HeldBy hb) {
        held_by = hb;
        return *this;
    }

    [[nodiscard]] EntityFilter& get_filter() { return filter; }
    [[nodiscard]] const EntityFilter& get_filter() const { return filter; }

   private:
    std::shared_ptr<Entity> held_item = nullptr;
    EntityFilter filter;
    IsItem::HeldBy held_by;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // s.ext(held_item, bitsery::ext::StdSmartPtr{});

        // TODO we only need this for debug info
        s.object(filter);
    }
};
