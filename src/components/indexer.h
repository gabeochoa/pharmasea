
#pragma once

#include "../engine/log.h"
#include "base_component.h"

struct Indexer : public BaseComponent {
    Indexer() : index(0), max_value(0), changed(true) {}
    explicit Indexer(int mx) : index(0), max_value(mx), changed(true) {}

    void increment() {
        if (max_value == 0) return;
        if (max_value == 1) return;

        index = (index + 1) % max_value;
        changed = true;
    }

    [[nodiscard]] bool value_same_as_last_render() const {
        return !value_changed_since_last_render();
    }
    [[nodiscard]] bool value_changed_since_last_render() const {
        return changed;
    }

    void mark_change_completed() { changed = false; }

    [[nodiscard]] int value() const { return index; }
    [[nodiscard]] int max() const { return max_value; }

    void update_max(int mx) { max_value = mx; }
    auto& set_value(int val) {
        index = val;
        return *this;
    }

    void increment_until_valid(const std::function<bool(int)>& validationFn) {
        for (int i = 0; i < max_value; i++) {
            if (validationFn(index)) return;
            increment();
        }

        log_warn(
            "You are using increment until valid, but your validation function "
            "likely never returns true");
    }

   private:
    int index;
    int max_value;
    bool changed;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(index);
        s.value4b(max_value);

        // s.value1b(changed);
    }
};
