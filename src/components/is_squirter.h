
#pragma once

#include "../vendor_include.h"
#include "base_component.h"

using EntityID = int;

struct IsSquirter : public BaseComponent {
    virtual ~IsSquirter() {}
    IsSquirter() : sq_time(1.f), sq_time_reset(1.f) {}

    void update(EntityID item_id, vec3 pickup_location) {
        held_item_id = item_id;
        pos = pickup_location;
    }

    void reset() {
        sq_time = sq_time_reset;
        working = false;
        held_item_id = -1;
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

    void set_drink_id(EntityID id) { held_drink_id = id; }
    [[nodiscard]] EntityID drink_id() const { return held_drink_id; }
    [[nodiscard]] EntityID item_id() const { return held_item_id; }
    [[nodiscard]] vec3 picked_up_at() const { return pos; }

   private:
    vec3 pos;
    EntityID held_item_id = -1;
    EntityID held_drink_id = -1;

    bool working = false;
    float sq_time;
    float sq_time_reset;

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                pos, held_drink_id, held_item_id, working, sq_time,
                sq_time_reset);
    }
};
CEREAL_REGISTER_TYPE(IsSquirter);
