
#pragma once

#include "../entities/entity_id.h"
#include "../entities/entity_ref.h"
#include "../vendor_include.h"
#include "base_component.h"

struct IsSquirter : public BaseComponent {
    IsSquirter() : sq_time(1.f), sq_time_reset(1.f) {}

    void update(EntityID item_id, vec3 pickup_location) {
        held_item.set_id(item_id);
        pos = pickup_location;
    }

    void reset() {
        sq_time = sq_time_reset;
        working = false;
        held_item.clear();
        pos = {0, 0, 0};
    }

    bool pass_time(float dt) {
        sq_time -= dt;
        if (sq_time <= 0) {
            reset();
            return true;
        }
        return false;
    }

    [[nodiscard]] bool is_working() const { return working; }
    [[nodiscard]] float pct() const { return sq_time / sq_time_reset; }
    [[nodiscard]] float time() const { return sq_time; }

    void set_drink_id(EntityID id) { held_drink.set_id(id); }
    [[nodiscard]] EntityID drink_id() const { return held_drink.id; }
    [[nodiscard]] EntityID item_id() const { return held_item.id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    vec3 pos;
    EntityRef held_item{};
    EntityRef held_drink{};

    bool working = false;
    float sq_time;
    float sq_time_reset;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.pos,                           //
            self.held_item,                     //
            self.held_drink,                    //
            self.working,                       //
            self.sq_time,                       //
            self.sq_time_reset                  //
        );
    }
};
