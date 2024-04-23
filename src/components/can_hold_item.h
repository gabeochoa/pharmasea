#pragma once

#include <exception>
#include <optional>
//

#include "../entity.h"
#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "has_subtype.h"
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
    explicit CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    // TODO add a comment for why we are storing the item as a shared_ptr
    CanHoldItem& update(std::shared_ptr<Entity> item, int entity_id) {
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
        if (held_item) {
            held_item->get<IsItem>().set_held_by(held_by, entity_id);
            last_held_id = held_item->id;
        }
        if (held_item && held_by == EntityType::Unknown) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item->id, item->name());
        }
        return *this;
    }

    [[nodiscard]] Entity& item() const { return *held_item; }
    [[nodiscard]] const Entity& const_item() const { return *held_item; }

    CanHoldItem& set_filter(EntityFilter ef) {
        filter = ef;
        return *this;
    }

    [[nodiscard]] bool can_hold(const Entity& item,
                                RespectFilter respect_filter) const {
        if (item.has<IsItem>()) {
            bool cbhb = item.get<IsItem>().can_be_held_by(held_by);
            // log_info("trying to pick up {} with {} and {} ",
            // item.name(), held_by, cbhb);
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
    [[nodiscard]] EntityType hb_type() const { return held_by; }

    [[nodiscard]] EntityID last_id() const { return last_held_id; }

   private:
    EntityID last_held_id = -1;
    std::shared_ptr<Entity> held_item = nullptr;
    EntityType held_by;
    EntityFilter filter;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(held_by);

        // we only need these for debug info
        s.ext(held_item, bitsery::ext::StdSmartPtr{});
        s.object(filter);
    }
};
