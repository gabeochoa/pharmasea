#pragma once

#include "../engine/log.h"
#include "ai_component.h"
#include "base_component.h"

struct AIPlayJukebox : public AIComponent {
    virtual ~AIPlayJukebox() {}

    [[nodiscard]] bool has_available_target() const {
        return target_id.has_value();
    }

    void unset_target() { target_id = {}; }
    void set_target(int id) { target_id = id; }
    [[nodiscard]] int id() const { return target_id.value(); }

    std::optional<int> target_id;
    vec2 position;

    float findSongTime = -1;
    void set_findSong_time(float pt) { findSongTime = pt; }
    [[nodiscard]] bool findSong(float dt) {
        findSongTime -= dt;
        return findSongTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};
