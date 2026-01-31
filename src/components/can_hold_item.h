#pragma once

#include <exception>
#include <memory>
#include <optional>
//

#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "../entity.h"
#include "../entity_id.h"
#include "../entity_ref.h"
#include "../entity_type.h"
#include "has_subtype.h"
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
    explicit CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

    [[nodiscard]] bool empty() const {
        // Treat stale IDs as empty to avoid asserting on missing entities.
        return !item();
    }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    CanHoldItem& update(std::shared_ptr<Entity> item, int entity_id) {
        if (!item) return update(nullptr, entity_id);
        return update(*item, entity_id);
    }

    CanHoldItem& update(Entity& item, int entity_id) {
        held_item.set(item);
        // Defensive: this should always be an item, but avoid hard-crashing if
        // an unexpected entity is passed in.
        if (item.has<IsItem>()) {
            item.get<IsItem>().set_held_by(held_by, entity_id);
        } else {
            log_error(
                "CanHoldItem::update: entity {} was set as held item but is "
                "missing IsItem",
                item.id);
        }
        last_held.set(item);
        if (held_by == EntityType::Unknown) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item.id, str(get_entity_type(item)));
        }
        return *this;
    }

    CanHoldItem& update(std::nullptr_t, int) {
        held_item.clear();
        return *this;
    }

    [[nodiscard]] OptEntity item() const;
    [[nodiscard]] OptEntity const_item() const;

    [[nodiscard]] EntityID item_id() const { return held_item.id; }

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

    [[nodiscard]] EntityID last_id() const { return last_held.id; }

   private:
    EntityRef last_held{};
    EntityRef held_item{};
    EntityType held_by;
    EntityFilter filter;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.held_by,                       //
            self.held_item,                     //
            self.last_held,                     //
            self.filter                         //
        );
    }
};
