
#pragma once

#include "../entity.h"
#include "base_component.h"

struct IsItemContainer : public BaseComponent {
    IsItemContainer() : item_type(EntityType::Unknown) {}

    explicit IsItemContainer(EntityType type) : item_type(type) {}
    virtual ~IsItemContainer() {}

    [[nodiscard]] virtual bool is_matching_item(std::shared_ptr<Item> item) {
        return check_type(*item, item_type);
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

   private:
    int gens = 0;
    int max_gens = -1;
    EntityType item_type;
    bool uses_indexer;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // s.text1b(item_type, MAX_ITEM_NAME);
        // s.value4b(max_gens);
    }
};
