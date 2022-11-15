
#pragma once

#include "astar.h"
#include "entityhelper.h"
#include "external_include.h"
//
#include "globals_register.h"
#include "job.h"
#include "person.h"
#include "sound_library.h"
#include "ui_color.h"

struct AIPerson : public Person {
    std::stack<std::shared_ptr<Job>> personal_queue;
    std::shared_ptr<Job> job;
    std::optional<vec2> local_target;

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
    // virtual float stagger_mod() override {
    //     return 0
    // pull from ailment
    // }
    virtual void render() const override {
        Person::render();

        if (GLOBALS.get<bool>("debug_ui_enabled")) {
            const float box_size = TILESIZE / 10.f;
            if (job && !job->path.empty()) {
                for (auto location : job->path) {
                    DrawCube(vec::to3(location), box_size, box_size, box_size,
                             BLUE);
                }
            }
        }
    }

    virtual vec3 update_xaxis_position(float dt) override {
        if (!job) {
            return this->raw_position;
        }
        if (!job->local.has_value()) {
            return this->raw_position;
        }
        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        // float stagger = this->stagger_mod() * dt;

        auto new_pos_x = this->raw_position;
        if (tar.x > this->raw_position.x) new_pos_x.x += speed;
        //-= stagger;
        if (tar.x < this->raw_position.x) new_pos_x.x -= speed;
        // += stagger;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        if (!job) {
            return this->raw_position;
        }
        if (!job->local.has_value()) {
            return this->raw_position;
        }
        vec2 tar = job->local.value();
        float speed = this->base_speed() * dt;
        // float stagger = this->stagger_mod() * dt;

        auto new_pos_z = this->raw_position;
        if (tar.y > this->raw_position.z) new_pos_z.z += speed;
        //-= stagger;
        if (tar.y < this->raw_position.z) new_pos_z.z -= speed;
        //+= stagger;
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

            // TODO make sure this == works reasonably
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
            // TODO
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

    virtual void update(float dt) override {
        if ((this->pushed_force.x != 0.0f      //
             || this->pushed_force.z != 0.0f)  //
            && job != nullptr) {
            this->job->path.clear();
            this->job->local = {};
            PlaySound(SoundLibrary::get().get("roblox"));
        }
        Person::update(dt);
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
        auto my_thread_id = std::this_thread::get_id();
        auto server_thread_id =
            GLOBALS.get_or_default("server_thread_id", std::thread::id());
        if (my_thread_id == server_thread_id) {
            std::cout << this->id << ": " << text << std::endl;
        }
    }

    virtual bool is_snappable() override { return true; }
};
