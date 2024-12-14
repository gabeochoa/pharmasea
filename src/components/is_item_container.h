
#pragma once

#include "../entity.h"
#include "base_component.h"

struct IsItemContainer : public BaseComponent {
    IsItemContainer() : item_type(EntityType::Unknown), uses_indexer(false) {}

    explicit IsItemContainer(EntityType type)
        : item_type(type), uses_indexer(false) {}
    virtual ~IsItemContainer() {}

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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // s.text1b(item_type, MAX_ITEM_NAME);
        // s.value4b(max_gens);
    }
};
