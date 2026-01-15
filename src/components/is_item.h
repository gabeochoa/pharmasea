

#pragma once

#include "../entity_type.h"
#include "base_component.h"

struct IsItem : public BaseComponent {
    void set_held_by(EntityType hb, int entity_id) {
        held_by_id = entity_id;
        held_by = hb;
    }

    [[nodiscard]] bool is_held_by(EntityType hb) const { return held_by == hb; }

    [[nodiscard]] bool is_not_held_by(EntityType hb) const {
        return !is_held_by(hb);
    }

    [[nodiscard]] bool can_be_held_by(EntityType hb) const {
        return hb_filter.test(static_cast<int>(hb));
    }

    IsItem& clear_hb_filter() {
        hb_filter.reset();
        return *this;
    }

    IsItem& set_hb_filter(const EntityType& type) {
        hb_filter.set(static_cast<int>(type));
        return *this;
    }

    IsItem& remove_hb_filter(const EntityType& type) {
        hb_filter.reset(static_cast<int>(type));
        return *this;
    }

    IsItem& set_hb_filter(const EntityTypeSet& hb) {
        hb_filter = hb;
        return *this;
    }

    [[nodiscard]] bool is_held() const {
        // might need to do something more sophisticated
        return held_by != EntityType::Unknown;
    }

    [[nodiscard]] int holder() const { return held_by_id; }

   private:
    int held_by_id = -1;
    EntityType held_by = EntityType::Unknown;
    // Default to all
    EntityTypeSet hb_filter = EntityTypeSet().set();

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                        //
            static_cast<BaseComponent&>(self)  //
        );
    }
};
