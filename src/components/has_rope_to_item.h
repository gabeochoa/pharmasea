

#pragma once

#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_ref.h"
#include "base_component.h"

struct HasRopeToItem : public BaseComponent {
    void clear() {
        for (EntityRef& ref : rope) {
            EntityHelper::markIDForCleanup(ref.id);
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
        EntityRef ref{};
        ref.set(i);
        rope.push_back(ref);
        rope_length = (int) rope.size();
    }

    [[nodiscard]] vec2 goal() const { return path_to; }

   private:
    vec2 path_to;
    bool generated = false;
    int rope_length = 0;
    std::vector<EntityRef> rope;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.path_to,                       //
            self.generated,                     //
            self.rope_length,                   //
            self.rope                           //
        );
    }
};
