

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
        raylib::Texture texture = TextureLibrary::get().get(icon_tex_name);
        raylib::DrawBillboard(cam.camera, texture,
                              vec3{position.x,                      //
                                   position.y + (TILESIZE * 1.5f),  //
                                   position.z},                     //
                              TILESIZE,                             //
                              raylib::WHITE);
    }
};

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;
    std::shared_ptr<Ailment> ailment;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Only things that need to be rendered, need to be serialized :)
        s.ext(*this, bitsery::ext::BaseClass<AIPerson>{});
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
        update_name(get_random_name());

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
        auto wait_and_return = [&]() {
            // Add the current job to the queue,
            // and then add the waiting job
            personal_queue.push(job);
            this->job.reset(new Job({
                .type = Wait,
                .timeToComplete = 1.f,
                .start = job->start,
                .end = job->start,
            }));
            return;
        };

        switch (job->state) {
            case Job::State::Initialize: {
                announce("starting a new wait in queue job");

                // Figure out which register to go to...

                // TODO replace with finding the one with the least people in it
                std::shared_ptr<Register> closest_target =
                    EntityHelper::getClosestMatchingEntity<Register>(
                        this->get<Transform>().as2(), TILESIZE * 100.f,
                        [](auto&&) { return true; });

                if (!closest_target) {
                    // TODO we need some kinda way to save this job,
                    // and come back to it later
                    // i think just putting a Job* unfinished in Job is probably
                    // enough
                    announce("Could not find a valid register");
                    job->state = Job::State::Initialize;
                    wait_and_return();
                    return;
                }

                job->data["register"] = closest_target.get();
                Customer* me = this;
                job->start = closest_target->get_next_queue_position(me);
                job->end = closest_target->tile_infront(1);
                job->spot_in_line = closest_target->position_in_line(me);
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                bool arrived = navigate_to(job->start);
                job->state = arrived ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart;
                return;
            }
            case Job::State::WorkingAtStart: {
                if (job->spot_in_line == 0) {
                    job->state = Job::State::HeadingToEnd;
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
                    job->state = Job::State::WorkingAtStart;
                    wait_and_return();
                    return;
                }

                // if our spot did change, then move forward
                announce(fmt::format("im moving up to {}", cur_spot_in_line));

                job->spot_in_line = cur_spot_in_line;

                if (job->spot_in_line == 0) {
                    job->state = Job::State::HeadingToEnd;
                    return;
                }

                // otherwise walk up one spot
                job->start = reg->tile_infront(job->spot_in_line);
                job->state = Job::State::WorkingAtStart;
                return;
            }
            case Job::State::HeadingToEnd: {
                bool arrived = navigate_to(job->end);
                job->state = arrived ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd;
                return;
            }
            case Job::State::WorkingAtEnd: {
                Register* reg = (Register*) job->data["register"];

                if (reg->held_item == nullptr) {
                    announce("my rx isnt ready yet");
                    wait_and_return();
                    return;
                }

                auto bag = dynamic_pointer_cast<Bag>(reg->held_item);
                if (!bag) {
                    announce("this isnt my rx (not a bag)");
                    wait_and_return();
                    return;
                }

                if (bag->empty()) {
                    announce("this bag is empty...");
                    wait_and_return();
                    return;
                }

                auto pill_bottle =
                    dynamic_pointer_cast<PillBottle>(bag->held_item);
                if (!pill_bottle) {
                    announce("this bag doesnt have my pills");
                    wait_and_return();
                    return;
                }

                this->held_item = reg->held_item;
                reg->held_item = nullptr;

                announce("got it");
                Customer* me = this;
                reg->leave_line(me);
                job->state = Job::State::Completed;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
        }
    }

    void wait(float dt) {
        switch (job->state) {
            case Job::State::Initialize: {
                announce("starting a new wait job");
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                job->state = Job::State::WorkingAtStart;
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
                job->timePassedInCurrentState += dt;
                if (job->timePassedInCurrentState >= job->timeToComplete) {
                    job->state = Job::State::Completed;
                    return;
                }
                // announce(fmt::format("waiting a little longer: {} => {} ",
                // job->timePassedInCurrentState,
                // job->timeToComplete));
                job->state = Job::State::WorkingAtEnd;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
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

    virtual void in_round_update(float dt) override {
        AIPerson::in_round_update(dt);

        // Register* reg = get_target_register();
        // if (reg) {
        // this->turn_to_face_entity(reg);
        // }
        //
        if (bubble.has_value())
            bubble.value().update(dt, this->get<Transform>().raw_position);
    }

    virtual void render_normal() const override {
        auto render_speech_bubble = [&]() {
            if (!this->bubble.has_value()) return;
            this->bubble.value().render();
        };
        AIPerson::render_normal();
        render_speech_bubble();
    }
};
