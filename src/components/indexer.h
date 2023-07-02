
#pragma once

#include "base_component.h"

struct Indexer : public BaseComponent {
    Indexer() : index(0), max_value(0), changed(true) {}
    explicit Indexer(int mx) : index(0), max_value(mx), changed(true) {}

    virtual ~Indexer() {}

    void increment() {
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

   private:
    int index;
    int max_value;
    bool changed;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(max_value);
        s.value4b(index);
        s.value1b(changed);
    }
};
