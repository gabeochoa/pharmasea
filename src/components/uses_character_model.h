

#pragma once

#include "base_component.h"

struct UsesCharacterModel : public BaseComponent {
    UsesCharacterModel() : index(0), changed(true) {}

    virtual ~UsesCharacterModel() {}

    void increment() {
        index = (index + 1) % character_models.size();
        changed = true;
    }

    [[nodiscard]] const std::string& fetch_model() const {
        return character_models[index];
    }

    [[nodiscard]] bool value_same_as_last_render() const { return !changed; }
    void mark_change_completed() { changed = false; }

   private:
    std::array<std::string, 3> character_models = {
        "character_duck",
        "character_dog",
        "character_bear",
    };
    int index;
    bool changed;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(index);
        s.value1b(changed);
    }
};