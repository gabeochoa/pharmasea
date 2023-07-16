

#pragma once

#include "../entity.h"
#include "base_component.h"

struct HasRopeToItem : public BaseComponent {
    virtual ~HasRopeToItem() {}

    void clear() {
        for (std::shared_ptr<Item> i : rope) {
            i->cleanup = true;
        }
        rope.clear();
        generated = false;
    }

    void mark_generated() { generated = true; }
    [[nodiscard]] bool was_generated() const { return generated; }

    void add(std::shared_ptr<Item> i) { rope.push_back(i); }

   private:
    bool generated = false;
    std::vector<std::shared_ptr<Item>> rope;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
    }
};
