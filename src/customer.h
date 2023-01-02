

#pragma once

#include "drawing_util.h"
#include "entityhelper.h"
#include "external_include.h"
//
#include "aiperson.h"
//
#include "ailment.h"
#include "camera.h"
#include "engine/texture_library.h"
#include "furniture/register.h"
#include "globals.h"
#include "job.h"
#include "names.h"
#include "text_util.h"

struct SpeechBubble {
    vec3 position;
    // TODO we arent using any string functionality can we swap to const char*?
    // perhaps in a typedef?
    std::string icon_tex_name;

    explicit SpeechBubble(std::string icon) : icon_tex_name(icon) {}

    void update(float, vec3 pos) { this->position = pos; }
    void render() const {
        GameCam cam = GLOBALS.get<GameCam>("game_cam");
        Texture texture = TextureLibrary::get().get(icon_tex_name);
        DrawBillboard(cam.camera, texture,
                      vec3{position.x,                      //
                           position.y + (TILESIZE * 1.5f),  //
                           position.z},                     //
                      TILESIZE,                             //
                      WHITE);
    }
};

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;
    std::shared_ptr<Ailment> ailment;

    void set_customer_name(std::string new_name) {
        name = new_name;
        customer_name_length = (int) new_name.size();
    }

   private:
    std::string name = "Customer";
    int customer_name_length;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Only things that need to be rendered, need to be serialized :)
        s.ext(*this, bitsery::ext::BaseClass<AIPerson>{});
        //
        s.value4b(customer_name_length);
        s.text1b(name, customer_name_length);
    }

   public:
    Customer() : AIPerson() { init(); }
    Customer(vec3 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {
        init();
    }
    Customer(vec2 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {
        init();
    }
    Customer(vec3 p, Color c) : AIPerson(p, c) { init(); }
    Customer(vec2 p, Color c) : AIPerson(p, c) { init(); }

    void init() {
        set_customer_name(get_random_name());

        // TODO turn back on ailments
        // ailment.reset(new Insomnia());
        // bubble = SpeechBubble(ailment->icon_name());
    }

    virtual float base_speed() override {
        float base_speed = 4.f;
        if (ailment) base_speed *= ailment->speed_multiplier();
        return base_speed;
    }

    virtual float stagger_mult() override {
        return ailment ? ailment->stagger() : 0.f;
    }

    void wait_in_queue(float) {
        // TODO the job api is kinda finicky, is there a way we can strengthen
        // the config and running to make it more fool proof?
        auto init_job = [&]() {
            if (job->initialized) return;

            // Figure out what register to go to

            // TODO replace with finding the one with the least people in it
            std::shared_ptr<Register> closest_target =
                EntityHelper::getClosestMatchingEntity<Register>(
                    vec::to2(this->position), TILESIZE * 100.f,
                    [](auto&&) { return true; });

            if (!closest_target) {
                // TODO we need some kinda way to save this job,
                // and come back to it later
                // i think just putting a Job* unfinished in Job is probably
                // enough
                announce("Could not find a valid register");
                personal_queue.push(job);

                this->job.reset(new Job({
                    .type = Wait,
                    .timeToComplete = 1.f,
                }));
                return;
            }

            job->initialized = true;
            job->data["register"] = closest_target.get();
            Customer* me = this;
            job->start = closest_target->get_next_queue_position(me);
            job->end = closest_target->tile_infront(1);
            job->spot_in_line = closest_target->position_in_line(me);
            announce(fmt::format("initialized job, our spot is {}",
                                 job->spot_in_line));
        };

        auto work_at_start = [&]() {
            if (!job->reached_start) return;
            if (job->start_completed) return;
            // ------ END -----

            if (job->spot_in_line == 0) {
                job->start_completed = true;
                return;
            }

            // Check the spot in front of us
            Register* reg = static_cast<Register*>(job->data["register"]);
            Customer* me = this;
            int cur_spot_in_line = reg->position_in_line(me);

            if (cur_spot_in_line == job->spot_in_line ||
                !reg->can_move_up(me)) {
                // We didnt move so just wait a bit before trying again
                announce(fmt::format("im just going to wait a bit longer"));

                // Add the current job to the queue,
                // and then add the waiting job
                personal_queue.push(job);

                this->job.reset(new Job({
                    .type = Wait,
                    .timeToComplete = 1.f,
                }));
                return;
            }

            // if our spot did change, then move forward
            announce(fmt::format("im moving up to {}", cur_spot_in_line));
            // Someone moved forward
            job->spot_in_line = cur_spot_in_line;
            if (job->spot_in_line == 0) {
                job->start_completed = true;
                return;
            }
            // otherwise walk up one spot
            job->reached_start = false;
            job->start = reg->tile_infront(job->spot_in_line);
        };

        auto work_at_end = [&]() {
            // cant work if complete
            if (job->is_complete) return;
            // cant work unless we got there
            if (!job->reached_end) return;
            // ------ END -----

            Register* reg = (Register*) job->data["register"];
            Customer* me = this;

            if (reg->held_item == nullptr) {
                announce("my rx isnt ready yet");

                // Add the current job to the queue,
                // and then add the waiting job
                personal_queue.push(job);
                this->job.reset(new Job({
                    .type = Wait,
                    .timeToComplete = 1.f,
                }));
                return;
            }

            this->held_item = reg->held_item;
            reg->held_item = nullptr;

            announce("got it");
            reg->leave_line(me);
            job->is_complete = true;
        };

        auto work_at_job = [&]() {
            work_at_start();
            work_at_end();
        };

        init_job();
        get_to_job();
        work_at_job();
    }

    void wait(float dt) {
        job->timePassedInCurrentState += dt;
        if (job->timePassedInCurrentState >= job->timeToComplete) {
            job->is_complete = true;
            return;
        }
    }

    virtual void get_starting_job() override {
        job.reset(new Job({
            .type = WaitInQueue,
        }));
    }

    virtual void process_job(float dt) override {
        switch (job->type) {
            case Wandering:
                wandering();
                break;
            case WaitInQueue:
                wait_in_queue(dt);
                break;
            case Wait:
                wait(dt);
                break;
            default:
                break;
        }
    }

    virtual void game_update(float dt) override {
        AIPerson::game_update(dt);

        // Register* reg = get_target_register();
        // if (reg) {
        // this->turn_to_face_entity(reg);
        // }
        //
        if (bubble.has_value()) bubble.value().update(dt, this->raw_position);
    }

    virtual void render_normal() const override {
        auto render_speech_bubble = [&]() {
            if (!this->bubble.has_value()) return;
            this->bubble.value().render();
        };
        AIPerson::render_normal();
        render_speech_bubble();
        DrawFloatingText(this->raw_position, Preload::get().font, name.c_str());
    }
};
