
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "entityhelper.h"
//
#include "components/has_base_speed.h"
#include "engine/keymap.h"

struct Person : public Entity {
   private:
    std::array<std::string, 3> character_models = {
        "character_duck",
        "character_dog",
        "character_bear",
    };
    int model_index = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
        s.value4b(model_index);
    }

   protected:
    Person() : Entity() {}

    Person(vec3 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {
        update_component();
    }
    Person(vec2 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {
        update_component();
    }
    Person(vec3 p, Color c) : Entity(p, c) { update_component(); }
    Person(vec2 p, Color c) : Entity(p, c) { update_component(); }

   public:
    void update_component() {
        addComponent<HasBaseSpeed>().update(10.f);
        get<Transform>().size =
            vec3{TILESIZE * 0.75f, TILESIZE * 0.75f, TILESIZE * 0.75f};
        // log_info("model index: {}", model_index);
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            // TODO fix this
            .model_name = character_models[model_index],
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });
    }

    void select_next_character_model() {
        model_index = (model_index + 1) % character_models.size();
    }
};
