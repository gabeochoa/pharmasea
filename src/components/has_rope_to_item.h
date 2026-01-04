

#pragma once

#include "../entity_helper.h"
#include "base_component.h"

using EntityID = int;

struct HasRopeToItem : public BaseComponent {
    void clear() {
        for (EntityID id : rope) {
            EntityHelper::markIDForCleanup(id);
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

    void add(Item& i) {
        rope.push_back(i.id);
        rope_length = (int) rope.size();
    }

    [[nodiscard]] vec2 goal() const { return path_to; }

   private:
    vec2 path_to;
    bool generated = false;
    int rope_length = 0;
    std::vector<EntityID> rope;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.path_to,                    //
            self.generated,                  //
            self.rope_length,                //
            self.rope                        //
        );
    }
};
