
#pragma once

#include "external_include.h"
//
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/log.h"
#include "engine/sound_library.h"
//
#include "entityhelper.h"
#include "job.h"
#include "person.h"

struct AIPerson : public Person {
    std::stack<std::shared_ptr<Job>> personal_queue;
    std::shared_ptr<Job> job;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Person>{});
        // Only things that need to be rendered, need to be serialized :)
    }

   public:
    AIPerson() : Person() {}

    AIPerson(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    AIPerson(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    AIPerson(vec3 p, Color c) : Person(p, c) {}
    AIPerson(vec2 p, Color c) : Person(p, c) {}

    virtual float base_speed() override { return 10.f; }

    virtual float stagger_mult() { return 0.f; }

    virtual void render_debug_mode() const override {
        Person::render_debug_mode();
        const float box_size = TILESIZE / 10.f;
        if (job && !job->path.empty()) {
            for (auto location : job->path) {
                DrawCube(vec::to3(location), box_size, box_size, box_size,
                         BLUE);
            }
        }
    }

    // TODO we are seeing issues where customers are getting stuck on corners
    // when turning. Before I feel like they were able to slide but it seems
    // like not anymore?
    virtual vec3 update_xaxis_position(float dt) override {
        if (!job || !job->local.has_value()) {
            return this->raw_position;
        }

        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        if (stagger_mult() != 0) speed *= stagger_mult();

        auto new_pos_x = this->raw_position;
        if (tar.x > this->raw_position.x) new_pos_x.x += speed;
        if (tar.x < this->raw_position.x) new_pos_x.x -= speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        if (!job || !job->local.has_value()) {
            return this->raw_position;
        }

        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        if (stagger_mult() != 0) speed *= stagger_mult();

        auto new_pos_z = this->raw_position;
        if (tar.y > this->raw_position.z) new_pos_z.z += speed;
        if (tar.y < this->raw_position.z) new_pos_z.z -= speed;
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
                        .start = vec::to2(this->position),
                        .end = target});
    }

    virtual void get_to_job() {
        auto ensure_has_path = [&]() {
            // already there
            if (job->reached_end) return;
            // already have a path
            if (!job->path.empty()) return;

            vec2 start = vec::to2(this->position);
            if (!job->reached_start && start == job->start) {
                job->reached_start = true;
                return;
            }
            if (job->start_completed && !job->reached_end &&
                start == job->end) {
                job->reached_end = true;
                return;
            }

            vec2 end = job->reached_start ? job->end : job->start;
            job->path = astar::find_path(
                start, end,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));
            // If path isnt solveable just mark it true and
            // make a new target
            if (job->path.size() == 0) {
                job->is_complete = true;
            }
        };

        auto ensure_local_target = [&]() {
            if (job->path.empty()) return;       // no path yet
            if (job->local.has_value()) return;  // already have local target
            // grab next local
            job->local = job->path.front();
            job->path.pop_front();
        };

        auto reset_local_target = [&]() {
            if (!job->local.has_value()) return;  // no local target yet

            // // TODO snap? vs normal? vs raw?
            if (vec::to2(this->snap_position()) == job->local.value()) {
                job->local.reset();
            }
        };

        auto reset_to_find_new_target = [&]() {
            if (!job->path.empty()) return;
            // path empty means we got to our current global target
            if (!job->reached_start &&
                vec::to2(this->snap_position()) == job->start) {
                job->reached_start = true;
                return;
            }
            // we reached the start and now our path is empty again
            // as long as we completed start we should be at the end
            if (job->start_completed &&
                vec::to2(this->snap_position()) == job->end) {
                job->reached_end = true;
                return;
            }
        };

        ensure_has_path();
        ensure_local_target();
        reset_local_target();
        reset_to_find_new_target();
    }

    virtual void wandering() {
        auto work_at_start = [&]() {
            if (!job->reached_start) return;
            if (job->start_completed) return;
            // For wandering type, this is enough
            job->start_completed = true;
        };

        auto work_at_end = [&]() {
            // cant work if complete
            if (job->is_complete) return;
            // cant work unless we got there
            if (!job->reached_end) return;
            // For wandering type, reaching the end is complete
            job->is_complete = true;
        };

        auto work_at_job = [&]() {
            work_at_start();
            work_at_end();
        };

        get_to_job();
        work_at_job();
    }

    virtual void get_starting_job() { job.reset(get_wandering_job()); }
    virtual void get_idle_job() { job.reset(get_wandering_job()); }

    virtual void find_new_job() {
        if (personal_queue.empty()) {
            get_idle_job();
            return;
        }
        job = personal_queue.top();
        personal_queue.pop();
    }

    virtual void process_job(float) {
        switch (job->type) {
            case Wandering:
                wandering();
                break;
            default:
                break;
        }
    }

    virtual void game_update(float dt) override {
        if ((this->pushed_force.x != 0.0f      //
             || this->pushed_force.z != 0.0f)  //
            && job != nullptr) {
            this->job->path.clear();
            this->job->local = {};
            SoundLibrary::get().play("roblox");
        }

        Person::game_update(dt);

        if (!job) {
            get_starting_job();
            return;
        }

        if (job->is_complete) {
            if (job->on_cleanup) job->on_cleanup(this, job.get());
            job.reset();
            find_new_job();
            return;
        }

        process_job(dt);
    }

    virtual void announce(std::string text) {
        // TODO have some way of distinguishing between server logs and regular
        // client logs
        if (is_server()) {
            log_info("server: {}: {}", this->id, text);
        } else {
            log_info("client: {}: {}", this->id, text);
        }
    }

    virtual bool is_snappable() override { return true; }
};
