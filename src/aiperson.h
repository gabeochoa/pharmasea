
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
        // TODO this doesnt work yet because job->path is not serialized
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

    void travel_on_path(vec2 me) {
        // did not arrive
        if (job->path.empty()) return;
        // Grab next local point to go to
        if (!job->local.has_value()) {
            job->local = job->path.front();
            job->path.pop_front();
        }
        // Go to local point
        if (job->local.value() == me) {
            job->local.reset();
            // announce(fmt::format("reached local point {} : {} ", me,
            // job->path.size()));
        }
        return;
    }

    bool navigate_to(vec2 goal) {
        vec2 me = vec::to2(this->position);
        if (me == goal) {
            // announce("reached goal");
            return true;
        }
        if (job->path.empty()) {
            job->path = astar::find_path(
                me, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));
            // announce(fmt::format("generated path from {} to {} with {}
            // steps", me, goal, job->path.size()));
        }
        travel_on_path(me);
        return false;
    }

    virtual void wandering() {
        switch (job->state) {
            case Job::State::Initialize: {
                announce("starting a new wandering job");
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                bool arrived = navigate_to(job->start);
                // TODO we cannot mutate this->job inside navigate because the
                // `job->state` below will change the new job and not the old
                // one this foot-gun might be solvable by passing in the global
                // job to the job processing function, then it wont change until
                // the next call
                if (job->path.size() == 0) {
                    personal_queue.push(job);
                    this->job.reset(new Job({
                        .type = Wandering,
                    }));
                    return;
                }
                job->state = arrived ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart;
                return;
            }
            case Job::State::WorkingAtStart: {
                job->state = Job::State::HeadingToEnd;
                return;
            }
            case Job::State::HeadingToEnd: {
                bool arrived = navigate_to(job->end);
                job->state = arrived ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd;
                return;
            }
            case Job::State::WorkingAtEnd: {
                job->state = Job::State::Completed;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
        }
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

        if (job->state == Job::State::Completed) {
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
        }
    }

    virtual bool is_snappable() override { return true; }
};
