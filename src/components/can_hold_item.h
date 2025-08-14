#pragma once

#include <exception>
#include <optional>
//

#include "base_component.h"
//
#include "../dataclass/entity_filter.h"
#include "has_subtype.h"
#include "is_item.h"
#include "type.h"

struct CanHoldItem : public BaseComponent {
	CanHoldItem() : held_by(EntityType::Unknown), filter(EntityFilter()) {}
	explicit CanHoldItem(EntityType hb) : held_by(hb), filter(EntityFilter()) {}

	virtual ~CanHoldItem() {}

	[[nodiscard]] bool empty() const { return held_item_id == -1; }
	// Whether or not this entity has something we can take from them
	[[nodiscard]] bool is_holding_item() const { return !empty(); }

	// Update to hold an item by id; -1 clears.
	CanHoldItem& update(EntityID item_id, int entity_id) {
		if (!empty()) {
			OptEntity prev = EntityHelper::getEntityForID(held_item_id);
			if (prev && !prev->cleanup && (prev->has<IsItem>() && !prev->get<IsItem>().is_held())) {
				log_warn(
					"you are updating the held item to null, but the old one isnt "
					"marked cleanup (and not being held) , this might be an issue "
					"if you are tring to "
					"delete it");
			}
		}

		held_item_id = item_id;
		if (!empty()) {
			OptEntity item = EntityHelper::getEntityForID(held_item_id);
			if (item) {
				item->get<IsItem>().set_held_by(held_by, entity_id);
				last_held_id = held_item_id;
			}
		}
		if (!empty() && held_by == EntityType::Unknown) {
			log_warn(
				"We never had our HeldBy set, so we are holding {} by UNKNOWN",
				item_id);
		}
		return *this;
	}

	[[nodiscard]] Entity& item() const { return EntityHelper::getEntityForID(held_item_id).asE(); }
	[[nodiscard]] const Entity& const_item() const { return EntityHelper::getEntityForID(held_item_id).asE(); }

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
	[[nodiscard]] EntityID item_id() const { return held_item_id; }

   private:
	EntityID last_held_id = -1;
	EntityID held_item_id = -1;
	EntityType held_by;
	EntityFilter filter;

	friend bitsery::Access;
	template<typename S>
	void serialize(S& s) {
		s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
		s.value4b(held_by);
		// Persist ids, not pointers
		s.value4b(held_item_id);
		s.object(filter);
	}
};
