
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "entityhelper.h"
//
#include "components/has_base_speed.h"
#include "components/uses_character_model.h"
#include "engine/keymap.h"

struct Person : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   protected:
    Person() : Entity() { update_component(); }

    Person(vec3 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {
        update_component();
    }

   public:
    void update_component() {
        get<Transform>().size =
            vec3{TILESIZE * 0.75f, TILESIZE * 0.75f, TILESIZE * 0.75f};

        addComponent<HasBaseSpeed>().update(10.f);
        // TODO why do we need the udpate() here?
        addComponent<ModelRenderer>().update(ModelInfo{
            .model_name = "character_duck",
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });

        addComponent<UsesCharacterModel>();
    }
};
