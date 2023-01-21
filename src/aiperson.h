
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

    void add_static_components() { addComponent<CanPerformJob>(); }

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

    // TODO we are seeing issues where customers are getting stuck on corners
    // when turning. Before I feel like they were able to slide but it seems
    // like not anymore?
    virtual vec3 update_xaxis_position(float dt) override {
        auto job = get<CanPerformJob>().job();

        if (!job || !job->local.has_value()) {
            return this->get<Transform>().raw_position;
        }

        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        if (stagger_mult() != 0) speed *= stagger_mult();

        auto new_pos_x = this->get<Transform>().raw_position;
        if (tar.x > this->get<Transform>().raw_position.x) new_pos_x.x += speed;
        if (tar.x < this->get<Transform>().raw_position.x) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        auto job = get<CanPerformJob>().job();

        if (!job || !job->local.has_value()) {
            return this->get<Transform>().raw_position;
        }

        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        if (stagger_mult() != 0) speed *= stagger_mult();

        auto new_pos_z = this->get<Transform>().raw_position;
        if (tar.y > this->get<Transform>().raw_position.z) new_pos_z.z += speed;
        if (tar.y < this->get<Transform>().raw_position.z) new_pos_z.z -= speed;
        return new_pos_z;
    }

    virtual Job* get_wandering_job() {
        // TODO add cooldown so that not all time is spent here
        int max_tries = 10;
        int range = 20;
        bool walkable = false;
        int i = 0;
        vec2 target;
        while (!walkable) {
            target = (vec2){1.f * randIn(-range, range),
                            1.f * randIn(-range, range)};
            walkable = EntityHelper::isWalkable(target);
            i++;
            if (i > max_tries) {
                return nullptr;
            }
        }
        return new Job({.type = Wandering,
                        .start = this->get<Transform>().as2(),
                        .end = target});
    }

    virtual void get_starting_job() {
        auto job = get<CanPerformJob>().job();
        job.reset(get_wandering_job());
    }
    virtual void get_idle_job() {
        auto job = get<CanPerformJob>().job();
        job.reset(get_wandering_job());
    }

    virtual void find_new_job() {
        auto job = get<CanPerformJob>().job();
        auto personal_queue = get<CanPerformJob>().job_queue();
        if (personal_queue.empty()) {
            get_idle_job();
            return;
        }
        job = personal_queue.top();
        personal_queue.pop();
    }

    virtual void in_round_update(float dt) override {
        auto job = get<CanPerformJob>().job();
        if ((this->pushed_force.x != 0.0f      //
             || this->pushed_force.z != 0.0f)  //
            && job != nullptr) {
            job->path.clear();
            job->local = {};
            SoundLibrary::get().play("roblox");
        }

        Person::in_round_update(dt);

        if (!job) {
            get_starting_job();
            return;
        }

        if (job->state == Job::State::Completed) {
            if (job->on_cleanup) job->on_cleanup(this, job.get());
            job.reset();
            find_new_job();
            return;
        }
    }

    virtual bool is_snappable() override { return true; }
};
