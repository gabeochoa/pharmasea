#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct IsTriggerArea : public BaseComponent {
    enum Type {
        Unset,
        Lobby_PlayGame,
        Progression_Option1,
        Progression_Option2,
    } type = Unset;

    IsTriggerArea(Type type)
        : type(type), wanted_entrants(1), completion_time_max(2.f) {}

    IsTriggerArea() : IsTriggerArea(Unset) {}

    virtual ~IsTriggerArea() {}

    [[nodiscard]] const std::string& title() const { return _title; }
    [[nodiscard]] bool should_wave() const {
        return has_min_matching_entrants();
    }

    [[nodiscard]] int max_entrants() const { return wanted_entrants; }

    [[nodiscard]] int active_entrants() const { return current_entrants; }

    [[nodiscard]] bool has_matching_entrants() const {
        return current_entrants == wanted_entrants;
    }

    [[nodiscard]] bool has_min_matching_entrants() const {
        return current_entrants >= wanted_entrants;
    }

    [[nodiscard]] float progress() const {
        return completion_time_passed / completion_time_max;
    }

    void increase_progress(float dt) {
        completion_time_passed =
            fminf(completion_time_max, completion_time_passed + dt);
    }

    void decrease_progress(float dt) {
        completion_time_passed = fmaxf(0, completion_time_passed - dt);
    }

    auto& update_progress_max(float amt) {
        completion_time_max = amt;
        return *this;
    }

    auto& update_title(const std::string& nt) {
        _title = nt;
        if ((int) _title.size() > max_title_length) {
            log_warn(
                "title for trigger area is too long, max is {} chars, you had "
                "{} ({})",
                max_title_length, _title.size(), nt);
            _title.resize(max_title_length);
        }
        return *this;
    }

    auto& update_entrants(int ne) {
        current_entrants = ne;
        return *this;
    }

    auto& update_max_entrants(int ne) {
        wanted_entrants = ne;
        return *this;
    }

    typedef std::function<void(const Entities&)> CompleteFn;
    auto& on_complete(CompleteFn cb) {
        complete_fn = cb;
        return *this;
    }
    [[nodiscard]] CompleteFn get_complete_fn() { return complete_fn; }

   private:
    int wanted_entrants = 1;
    int current_entrants = 0;
    std::string _title;
    int max_title_length = 20;

    float completion_time_max = 0.f;
    float completion_time_passed = 0.f;

    CompleteFn complete_fn;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(wanted_entrants);
        s.value4b(current_entrants);

        // These two are needed for drawing the percentage
        // TODO store the pct instead of sending two ints?
        s.value4b(completion_time_max);
        s.value4b(completion_time_passed);

        s.value4b(max_title_length);
        s.text1b(_title, max_title_length);
    }
};
