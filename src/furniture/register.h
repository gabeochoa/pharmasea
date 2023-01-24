
#pragma once

#include "../external_include.h"
//
#include "../components/has_waiting_queue.h"
#include "../engine/assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct RegisterNextQueuePosition : Person {
    RegisterNextQueuePosition(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    RegisterNextQueuePosition(vec3 p, Color c) : Person(p, c) {}
    RegisterNextQueuePosition(vec2 p, Color c) : Person(p, c) {}
};

struct Register : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
        // Only need to serialize things that are needed for render
    }

   public:
    Register() : Furniture() {}
    explicit Register(vec2 pos)
        : Furniture(pos, ui::color::grey, ui::color::grey) {
        update_model();
    }

    void update_model() {
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
};
