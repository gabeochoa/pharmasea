

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
        rope_length = (int) rope.size();
        generated = false;
    }

    void mark_generated(vec2 p) {
        path_to = p;
        generated = true;
    }
    [[nodiscard]] bool was_generated() const { return generated; }

    void add(std::shared_ptr<Item> i) {
        rope.push_back(i);
        rope_length = (int) rope.size();
    }

    [[nodiscard]] vec2 goal() const { return path_to; }

   private:
    vec2 path_to;
    bool generated = false;
    int rope_length;
    std::vector<std::shared_ptr<Item>> rope;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(path_to);
        s.value1b(generated);
        s.value4b(rope_length);

        s.container(rope, rope_length,
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });
    }
};
