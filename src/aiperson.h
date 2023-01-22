
#pragma once

#include "external_include.h"
//
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/log.h"
#include "engine/sound_library.h"
//
#include "components/can_perform_job.h"
#include "entityhelper.h"
#include "job.h"
#include "person.h"

struct AIPerson : public Person {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Person>{});
        // Only things that need to be rendered, need to be serialized :)
    }

    void add_static_components() {
        addComponent<CanPerformJob>().update(Wandering, Wandering);
    }

   public:
    AIPerson() : Person() { add_static_components(); }
    AIPerson(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {
        add_static_components();
    }
    AIPerson(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {
        add_static_components();
    }
    AIPerson(vec3 p, Color c) : Person(p, c) { add_static_components(); }
    AIPerson(vec2 p, Color c) : Person(p, c) { add_static_components(); }

    virtual float base_speed() override { return 10.f; }

    virtual float stagger_mult() { return 0.f; }

    virtual bool is_snappable() override { return true; }
};
