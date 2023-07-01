#pragma once

#include "base_component.h"

struct IsTriggerArea : public BaseComponent {
    virtual ~IsTriggerArea() {}

    [[nodiscard]] const std::string& title() const { return _title; }
    [[nodiscard]] bool should_wave() const {
        log_info("should wave? cur{} wanted{}", current_entrants,
                 wanted_entrants);
        return has_min_matching_entrants();
    }
    [[nodiscard]] bool has_matching_entrants() const {
        return current_entrants == wanted_entrants;
    }

    [[nodiscard]] bool has_min_matching_entrants() const {
        return current_entrants >= wanted_entrants;
    }

    auto& update_title(const std::string& nt) {
        _title = nt;
        if ((int) _title.size() > max_title_length) {
            log_warn(
                "title for trigger area is too long, max is {} chars, you had "
                "{}",
                max_title_length, _title.size());
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

    auto& increment_entrants() {
        if (current_entrants < wanted_entrants) current_entrants += 100;
        return *this;
    }

    auto& decrement_entrants() {
        if (current_entrants > 0) current_entrants -= 1;
        return *this;
    }

   private:
    int wanted_entrants;
    int current_entrants;
    std::string _title;
    int max_title_length = 20;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(wanted_entrants);
        s.value4b(current_entrants);

        s.value4b(max_title_length);
        s.text1b(_title, max_title_length);
    }
};
