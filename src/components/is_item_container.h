
#pragma once

#include "../entity.h"
#include "base_component.h"

constexpr int MAX_ITEM_NAME = 20;

struct IsItemContainer : public BaseComponent {
    IsItemContainer() : item_type("") {}

    IsItemContainer(const std::string& name) : item_type(name) {
        if ((int) item_type.size() > MAX_ITEM_NAME) {
            log_warn(
                "Your item name {} is longer than what we can serialize ({} "
                "chars) ",
                item_type, MAX_ITEM_NAME);
        }
    }
    virtual ~IsItemContainer() {}

    [[nodiscard]] virtual bool is_matching_item(
        std::shared_ptr<Item> item = nullptr) {
        if (item_type.empty()) {
            log_warn(
                "You created an item container but didnt specify which type to "
                "match, so we are going to match everything");
            return true;
        }
        return check_name(*item, item_type.c_str());
    }

    [[nodiscard]] const std::string& type() const { return item_type; }

    IsItemContainer& set_max_generations(int mx) {
        max_gens = mx;
        return *this;
    }

    [[nodiscard]] bool hit_max() const { return gens >= max_gens; }
    IsItemContainer& increment() {
        gens++;
        return *this;
    }

   private:
    int gens = 0;
    int max_gens = -1;
    std::string item_type;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.text1b(item_type, MAX_ITEM_NAME);
        s.value4b(max_gens);
    }
};
