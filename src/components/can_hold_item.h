#pragma once

#include <exception>
#include <memory>
#include <optional>
//

#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_type.h"
#include "has_subtype.h"
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
    explicit CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

    virtual ~CanHoldItem() {}

    // NOTE: A handle can be "syntactically valid" (slot != INVALID_SLOT) but
    // still be stale (entity deleted / generation bumped). Treat stale handles
    // as empty so gameplay doesn't crash or spin on error paths.
    [[nodiscard]] bool empty() const {
        if (held_item_handle.is_invalid()) return true;
        return !EntityHelper::resolve(held_item_handle).has_value();
    }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    CanHoldItem& update(std::shared_ptr<Entity> item, int entity_id) {
        if (!item) return update(nullptr, entity_id);
        return update(*item, entity_id);
    }

    CanHoldItem& update(Entity& item, int entity_id) {
        held_item_handle = EntityHelper::handle_for(item);
        item.get<IsItem>().set_held_by(held_by, entity_id);
        last_held_handle = held_item_handle;
        if (held_by == EntityType::Unknown) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item.id, str(get_entity_type(item)));
        }
        return *this;
    }

    CanHoldItem& update(std::nullptr_t, int) {
        held_item_handle = EntityHandle::invalid();
        return *this;
    }

    [[nodiscard]] Entity& item() const;
    [[nodiscard]] const Entity& const_item() const;

    [[nodiscard]] EntityHandle item_handle() const { return held_item_handle; }
    // Legacy compatibility - resolve handle to get ID
    [[nodiscard]] EntityID item_id() const {
        auto opt = EntityHelper::resolve(held_item_handle);
        return opt ? opt->id : entity_id::INVALID;
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

    [[nodiscard]] EntityHandle last_handle() const { return last_held_handle; }
    // Legacy compatibility
    [[nodiscard]] EntityID last_id() const {
        auto opt = EntityHelper::resolve(last_held_handle);
        return opt ? opt->id : entity_id::INVALID;
    }

   private:
    EntityHandle last_held_handle = EntityHandle::invalid();
    EntityHandle held_item_handle = EntityHandle::invalid();
    EntityType held_by;
    EntityFilter filter;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(held_by);
        s.object(held_item_handle);
        s.object(last_held_handle);
        s.object(filter);
    }
};
