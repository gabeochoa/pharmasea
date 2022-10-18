

#pragma once

#include "entityhelper.h"
#include "external_include.h"
//
#include "aiperson.h"
//
#include "app.h"
#include "camera.h"
#include "furniture/register.h"
#include "globals.h"
#include "job.h"
#include "text_util.h"
#include "texture_library.h"

struct SpeechBubble {
    vec3 position;
    std::string icon_tex_name;

    SpeechBubble() { icon_tex_name = "jug"; }

    void update(float, vec3 pos) { this->position = pos; }
    void render() const {
        GameCam cam = GLOBALS.get<GameCam>("game_cam");
        {
            Texture texture = TextureLibrary::get().get("bubble");
            DrawBillboard(cam.camera, texture,
                          vec3{position.x,                      //
                               position.y + (TILESIZE * 1.5f),  //
                               position.z},                     //
                          TILESIZE,                             //
                          WHITE);
        }
    }
};

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;

    std::string name = "Customer";

    Customer(vec3 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec2 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec3 p, Color c) : AIPerson(p, c) {}
    Customer(vec2 p, Color c) : AIPerson(p, c) {}

    virtual float base_speed() override { return 3.5f; }

    void wait_in_queue(float dt) {
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
            Register* reg = (Register*) job->data["register"];
            Customer* me = this;
            int cur_spot_in_line = reg->position_in_line(me);

            if (cur_spot_in_line == job->spot_in_line) {
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

            if (!reg->can_move_up(me)) {
                // We didnt move so just wait a bit before trying again
                announce(fmt::format("guy in front didnt move :( "));

                // Add the current job to the queue,
                // and then add the waiting job
                personal_queue.push(job);

                this->job.reset(new Job({
                    .type = Wait,
                    .timeToComplete = 0.5f,
                }));
                return;
            }

            // if our spot did change, then move forward
            announce(fmt::format("pog im moving up to {}", cur_spot_in_line));
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

            job->timeToComplete = 0.25f;
            job->timePassedInCurrentState += dt;
            if (job->timePassedInCurrentState < job->timeToComplete) {
                return;
            }

            announce("job complete");

            Register* reg = (Register*) job->data["register"];
            Customer* me = this;
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

    virtual void update(float dt) override {
        AIPerson::update(dt);

        // Register* reg = get_target_register();
        // if (reg) {
        // this->turn_to_face_entity(reg);
        // }
        //
        // // TODO just for debug purposes
        // if (!bubble.has_value()) {
        // bubble = SpeechBubble();
        // } else {
        // bubble.value().update(dt, this->raw_position);
        // }
    }

    virtual void render() const override {
        auto render_speech_bubble = [&]() {
            if (!this->bubble.has_value()) return;
            this->bubble.value().render();
        };

        auto render_name = [&]() {
            rlPushMatrix();
            rlTranslatef(              //
                this->raw_position.x,  //
                0.f,                   //
                this->raw_position.z   //
            );
            rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);

            rlTranslatef(          //
                -0.5f * TILESIZE,  //
                0.f,               //
                -1.05f * TILESIZE  // this is Y
            );

            DrawText3D(               //
                Preload::get().font,  //
                name.c_str(),         //
                {0.f},                //
                96,                   // font size
                4,                    // font spacing
                4,                    // line spacing
                true,                 // backface
                BLACK);

            rlPopMatrix();
        };

        AIPerson::render();
        render_speech_bubble();
        render_name();
    }
};
