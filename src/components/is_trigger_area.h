#pragma once

#include "../engine/is_server.h"
#include "../engine/log.h"
#include "base_component.h"

struct IsTriggerArea;

using ValidationResult = std::pair<bool, std::string>;
using ValidationFn = std::function<ValidationResult(const IsTriggerArea&)>;

struct IsTriggerArea : public BaseComponent {
    enum Type {
        Unset,
        Lobby_PlayGame,
        Lobby_ModelTest,
        Progression_Option1,
        Progression_Option2,
        Store_BackToPlanning,
    } type = Unset;

    explicit IsTriggerArea(Type type)
        : type(type), wanted_entrants(1), completion_time_max(2.f) {}

    IsTriggerArea() : IsTriggerArea(Unset) {}

    virtual ~IsTriggerArea() {}

    [[nodiscard]] const std::string& title() const { return _title; }
    [[nodiscard]] const std::string& subtitle() const { return _subtitle; }
    [[nodiscard]] bool should_wave() const {
        return has_min_matching_entrants() && should_progress();
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

    auto& update_subtitle(const std::string& nt) {
        _subtitle = nt;
        if ((int) _subtitle.size() > max_title_length) {
            log_warn(
                "subtitle for trigger area is too long, max is {} chars, you "
                "had "
                "{} ({})",
                max_title_length, _subtitle.size(), nt);
            _subtitle.resize(max_title_length);
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

    void set_validation_fn(const ValidationFn& cb) { validation_cb = cb; }

    [[nodiscard]] bool should_progress() const {
        if (is_server()) {
            last_validation_result =
                validation_cb ? validation_cb(*this) : std::pair{true, ""};
        }
        // If we are the client, then just use the serialized value
        // since we cant run the validation cb
        return last_validation_result.first;
    }

    [[nodiscard]] const std::string& validation_msg() const {
        return last_validation_result.second;
    }

   private:
    // This is mutable because we cant serialize std::funciton
    // and we need a way to send the values up.
    // it doesnt modify anything about the trigger area
    mutable ValidationResult last_validation_result = std::pair{true, ""};
    ValidationFn validation_cb = nullptr;

    int wanted_entrants = 1;
    int current_entrants = 0;
    std::string _title;
    std::string _subtitle;
    int max_title_length = 20;

    float completion_time_max = 0.f;
    float completion_time_passed = 0.f;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(last_validation_result.first);
        s.text1b(last_validation_result.second, 100);

        s.value4b(wanted_entrants);
        s.value4b(current_entrants);

        // These two are needed for drawing the percentage
        // TODO store the pct instead of sending two ints?
        s.value4b(completion_time_max);
        s.value4b(completion_time_passed);

        s.value4b(type);

        s.value4b(max_title_length);
        s.text1b(_title, max_title_length);
        s.text1b(_subtitle, max_title_length);
    }
};
