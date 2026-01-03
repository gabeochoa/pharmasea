
#pragma once

#include "../engine/log.h"
#include "../strings.h"
#include "../vendor_include.h"
#include "base_component.h"

using afterhours::Entity;

struct SpawnInfo {
    vec2 location;
    bool is_first_this_round;
};

using SpawnFn = std::function<void(Entity&, const SpawnInfo&)>;
using ValidationSpawnFn = std::function<bool(Entity&, const SpawnInfo&)>;

struct IsSpawner : public BaseComponent {
    [[nodiscard]] bool hit_max() const { return num_spawned >= max_spawned; }
    [[nodiscard]] int get_num_spawned() const { return num_spawned; }
    [[nodiscard]] int get_max_spawned() const { return max_spawned; }

    auto& reset_num_spawned() {
        num_spawned = 0;
        // set this to 0 so that the first guy immediately spawns
        countdown = 0.f;
        return *this;
    }

    auto& set_fn(const SpawnFn& fn) {
        spawn_fn = fn;
        return *this;
    }

    auto& set_validation_fn(const ValidationSpawnFn& fn) {
        validation_spawn_fn = fn;
        return *this;
    }

    auto& enable_prevent_duplicates() {
        prevent_duplicate_spawns = true;
        return *this;
    }

    auto& set_total(int mx) {
        max_spawned = mx;
        return *this;
    }

    auto& increase_total(int mx) {
        max_spawned += mx;
        return *this;
    }

    // might need a thing to specify the units
    auto& set_time_between(float s) {
        spread = s;
        countdown = 0.f;
        return *this;
    }

    bool pass_time(float dt) {
        if (hit_max()) {
            // log_info("hit max spawn {} {}", num_spawned, max_spawned);
            return false;
        }
        if (!spawn_fn) {
            log_warn("calling pass_time without a valid spawn function");
            return false;
        }

        if (countdown <= 0) {
            return true;
        }
        countdown -= dt;
        return false;
    }

    void post_spawn_reset() {
        // we do += instead of = because
        // we might overshoot when ffwding
        countdown += spread;
    }

    [[nodiscard]] float get_pct() const { return countdown / (spread * 1.f); }

    void spawn(Entity& entity, SpawnInfo info) {
        spawn_fn(entity, info);
        num_spawned++;
    }

    [[nodiscard]] bool validate(Entity& entity, SpawnInfo info) {
        return validation_spawn_fn ? validation_spawn_fn(entity, info) : true;
    }

    [[nodiscard]] bool prevent_dupes() const {
        return prevent_duplicate_spawns;
    }

    auto& set_spawn_sound(strings::sounds::SoundId str) {
        spawn_sound = str;
        return *this;
    }
    [[nodiscard]] bool has_spawn_sound() const {
        return spawn_sound != strings::sounds::SoundId::None;
    }
    [[nodiscard]] strings::sounds::SoundId get_spawn_sound() const {
        return spawn_sound;
    }

    auto& enable_show_progress() {
        showsProgressBar = true;
        return *this;
    }
    [[nodiscard]] bool show_progress() const { return showsProgressBar; }

   private:
    bool showsProgressBar = false;
    bool prevent_duplicate_spawns = false;
    int max_spawned = 0;
    float spread = 0;

    int num_spawned = 0;
    float countdown = 0;
    SpawnFn spawn_fn;
    ValidationSpawnFn validation_spawn_fn;

    strings::sounds::SoundId spawn_sound = strings::sounds::SoundId::None;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // We likely dont need to serialize anything because it should be
        // all server side info
        //
        // TODO add macro to only show these for debug builds
        // Debug only
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.showsProgressBar,           //
            self.countdown,                  //
            self.spread,                     //
            self.num_spawned,                //
            self.max_spawned                 //
        );
    }
};
