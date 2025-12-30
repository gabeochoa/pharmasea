#pragma once

#include <exception>
#include <optional>
//

#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_type.h"
#include "has_subtype.h"
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
    explicit CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item_id == EntityID::INVALID; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    // Keep API: take shared_ptr, but store only the handle (id).
    CanHoldItem& update(std::shared_ptr<Entity> item, int entity_id) {
        held_item_id = item ? item->id : EntityID::INVALID;
        if (item) {
            item->get<IsItem>().set_held_by(held_by, entity_id);
            last_held_id = item->id;
        }
        if (item && held_by == EntityType::Unknown) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item->id, str(get_entity_type(*item)));
        }
        return *this;
    }

    [[nodiscard]] Entity& item() const {
        return EntityHelper::getEnforcedEntityForID(held_item_id);
    }
    [[nodiscard]] const Entity& const_item() const {
        return EntityHelper::getEnforcedEntityForID(held_item_id);
    }

    [[nodiscard]] EntityID item_id() const { return held_item_id; }

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
    EntityID last_held_id = EntityID::INVALID;
    EntityID held_item_id = EntityID::INVALID;
    EntityType held_by;
    EntityFilter filter;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(held_by);
        s.value4b(held_item_id);
        s.value4b(last_held_id);
        s.object(filter);
    }
};
