
#pragma once

#include "../entities/entity.h"
#include "base_component.h"

struct IsItemContainer : public BaseComponent {
    IsItemContainer() : item_type(EntityType::Unknown), uses_indexer(false) {}

    explicit IsItemContainer(EntityType type)
        : item_type(type), uses_indexer(false) {}

    [[nodiscard]] virtual bool is_matching_item(const Item& item) const {
        return check_type(item, item_type);
    }

    [[nodiscard]] EntityType type() const { return item_type; }
    [[nodiscard]] bool should_use_indexer() const { return uses_indexer; }

    IsItemContainer& set_max_generations(int mx) {
        max_gens = mx;
        return *this;
    }

    IsItemContainer& set_uses_indexer(bool v) {
        uses_indexer = v;
        return *this;
    }

    IsItemContainer& set_item_type(EntityType type) {
        item_type = type;
        return *this;
    }

    [[nodiscard]] bool hit_max() const {
        return max_gens < 0 ? false : gens >= max_gens;
    }

    IsItemContainer& increment() {
        gens++;
        return *this;
    }

    void reset_generations() { gens = 0; }

    void enable_table_when_enable() { is_table_when_empty = true; }
    [[nodiscard]] bool table_when_empty() const { return is_table_when_empty; }

   private:
    int gens = 0;
    int max_gens = -1;
    EntityType item_type;
    bool uses_indexer;
    bool is_table_when_empty = false;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.gens,                          //
            self.max_gens                       //
        );
        // item_type, uses_indexer, is_table_when_empty are set by entity type
        // during creation and don't need to be serialized - containers reset
        // empty on load
    }
};
