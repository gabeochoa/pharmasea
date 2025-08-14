

#pragma once

#include "../engine/random_engine.h"
#include "../strings.h"
#include "base_component.h"

struct UsesCharacterModel : public BaseComponent {
    UsesCharacterModel() : index(0), changed(true) {}

    virtual ~UsesCharacterModel() {}

    auto& switch_to_random_model() {
        index = RandomEngine::get().get_index(strings::character_models);
        changed = true;
        return *this;
    }

    void increment() {
        index = (index + 1) % strings::character_models.size();
        changed = true;
    }

    [[nodiscard]] std::string fetch_model_name() const {
        return std::string(strings::character_models[index]);
    }

    [[nodiscard]] bool value_same_as_last_render() const {
        return !value_changed_since_last_render();
    }
    [[nodiscard]] bool value_changed_since_last_render() const {
        return changed;
    }

    void mark_change_completed() { changed = false; }

    [[nodiscard]] int index_server_only() const { return index; }

    void update_index_CLIENT_ONLY(int newind) {
        index = newind % strings::character_models.size();
        changed = true;
    }

   private:
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
