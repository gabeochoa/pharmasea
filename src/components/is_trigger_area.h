#pragma once

#include "../building_locations.h"
#include "../engine/is_server.h"
#include "../engine/log.h"
#include "../strings.h"
#include "base_component.h"

struct IsTriggerArea;

// This is not a translated string because we serialize it and it needs to be in
// the local language of the client
using ValidationResult = std::pair<bool, strings::i18n>;
using ValidationFn = std::function<ValidationResult(const IsTriggerArea&)>;

struct IsTriggerArea : public BaseComponent {
    enum Type {
        Unset,
        Lobby_PlayGame,
        Lobby_ModelTest,
        Progression_Option1,
        Progression_Option2,
        Store_BackToPlanning,
        Store_Reroll,
        ModelTest_BackToLobby,

        Lobby_LoadSave,
        LoadSave_BackToLobby,

        // Load/Save room actions (slot number stored in HasSubtype.type_index).
        LoadSave_LoadSlot,
        LoadSave_ToggleDeleteMode,

        Planning_SaveSlot,
    } type = Unset;

    enum struct EntrantsRequired {
        All,
        AllInBuilding,
        One,
    } entrantsRequired = EntrantsRequired::All;
    std::optional<Building> building;

    explicit IsTriggerArea(Type type)
        : type(type),
          wanted_entrants(1),
          completion_time_max(2.f),
          cooldown_time_max(0.f) {}

    IsTriggerArea() : IsTriggerArea(Unset) {}

    [[nodiscard]] const TranslatableString& title() const { return _title; }
    [[nodiscard]] const TranslatableString& subtitle() const {
        return _subtitle;
    }
    [[nodiscard]] bool should_wave() const {
        return has_min_matching_entrants() && should_progress();
    }

    [[nodiscard]] int all_possible_entrants() const { return wanted_entrants; }
    [[nodiscard]] int min_req_entrants() const {
        switch (entrantsRequired) {
            case EntrantsRequired::All:
                return all_possible_entrants();
            case EntrantsRequired::AllInBuilding:
                return current_in_building;
            case EntrantsRequired::One:
                return 1;
        }
        return all_possible_entrants();
    }

    [[nodiscard]] int active_entrants() const { return current_entrants; }

    [[nodiscard]] bool has_min_matching_entrants() const {
        switch (entrantsRequired) {
            case EntrantsRequired::AllInBuilding:
                return current_entrants >= 1 &&
                       current_entrants >= min_req_entrants();
            case EntrantsRequired::All:
                return current_entrants >= min_req_entrants();
            case EntrantsRequired::One:
                return current_entrants == 1;
        }
        return false;
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

    auto& update_cooldown_max(float amt) {
        cooldown_time_max = amt;
        return *this;
    }

    void reset_cooldown() { cooldown_time_passed = cooldown_time_max; }

    void increase_cooldown(float dt) {
        cooldown_time_passed =
            fminf(cooldown_time_max, cooldown_time_passed + dt);
    }

    void decrease_cooldown(float dt) {
        cooldown_time_passed = fmaxf(0, cooldown_time_passed - dt);
    }

    [[nodiscard]] bool is_on_cooldown() const {
        return cooldown_time_passed > 0.f;
    }

    auto& update_title(const TranslatableString& nt) {
        _title = nt;
        if ((int) _title.size() > max_title_length) {
            log_warn(
                "title for trigger area is too long, max is {} chars, you had "
                "{} ({})",
                max_title_length, _title.size(), nt.debug());
            _title.resize(max_title_length);
        }
        return *this;
    }

    auto& update_subtitle(const TranslatableString& nt) {
        _subtitle = nt;
        if ((int) _subtitle.size() > max_title_length) {
            log_warn(
                "subtitle for trigger area is too long, max is {} chars, you "
                "had "
                "{} ({})",
                max_title_length, _subtitle.size(), nt.debug());
            _subtitle.resize(max_title_length);
        }
        return *this;
    }

    auto& update_entrants(int ne) {
        current_entrants = ne;
        return *this;
    }

    auto& update_entrants_in_building(int ne) {
        current_in_building = ne;
        return *this;
    }

    auto& update_all_entrants(int ne) {
        wanted_entrants = ne;
        return *this;
    }

    auto& set_validation_fn(const ValidationFn& cb) {
        validation_cb = cb;
        return *this;
    }

    [[nodiscard]] bool should_progress() const {
        if (is_on_cooldown()) {
            return false;
        }
        if (is_server()) {
            last_validation_result =
                validation_cb ? validation_cb(*this)
                              : std::pair{true, strings::i18n::Empty};
        }
        // If we are the client, then just use the serialized value
        // since we cant run the validation cb
        return last_validation_result.first;
    }

    [[nodiscard]] const strings::i18n& validation_msg() const {
        return last_validation_result.second;
    }

    auto& set_required_entrants_type(
        EntrantsRequired req, const std::optional<Building>& _building = {}) {
        entrantsRequired = req;
        building = _building;

        if (entrantsRequired == EntrantsRequired::AllInBuilding &&
            !building.has_value()) {
            log_error(
                "You selected all in building but didnt pass in a building");
        }
        return *this;
    }

   private:
    // This is mutable because we cant serialize std::funciton
    // and we need a way to send the values up.
    // it doesnt modify anything about the trigger area
    mutable ValidationResult last_validation_result =
        std::pair{true, strings::i18n::Empty};
    ValidationFn validation_cb = nullptr;

    int wanted_entrants = 1;
    int current_entrants = 0;
    int current_in_building = 0;
    TranslatableString _title;
    TranslatableString _subtitle;
    int max_title_length = TranslatableString::MAX_LENGTH;

    float completion_time_max = 0.f;
    float completion_time_passed = 0.f;

    float cooldown_time_max = 0.f;
    float cooldown_time_passed = 0.f;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // These two are needed for drawing the percentage
        // TODO store the pct instead of sending two ints?
        return archive(                          //
            static_cast<BaseComponent&>(self),   //
            self.last_validation_result.first,   //
            self.last_validation_result.second,  //
            self.wanted_entrants,                //
            self.current_entrants,               //
            self.current_in_building,            //
            self.entrantsRequired,               //
            self.building,                       //
            self.completion_time_max,            //
            self.completion_time_passed,         //
            self.type,                           //
            self.max_title_length,               //
            self._title,                         //
            self._subtitle                       //
        );
    }
};
