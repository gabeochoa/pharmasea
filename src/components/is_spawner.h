
#pragma once

#include "base_component.h"

struct Entity;

typedef std::function<void(Entity&, vec2)> SpawnFn;
typedef std::function<bool(Entity&, vec2)> ValidationSpawnFn;

struct IsSpawner : public BaseComponent {
    virtual ~IsSpawner() {}

    [[nodiscard]] bool hit_max() const { return num_spawned >= max_spawned; }
    [[nodiscard]] int get_num_spawned() const { return num_spawned; }
    [[nodiscard]] int get_max_spawned() const { return max_spawned; }

    auto& reset_num_spawned() {
        // TODO add max to progression_manager
        num_spawned = 0;
        // set this to 0 so that the first guy immediately spawns
        countdown = 0.f;
        return *this;
    }

    auto& set_fn(SpawnFn fn) {
        spawn_fn = fn;
        return *this;
    }

    auto& set_validation_fn(ValidationSpawnFn fn) {
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

    // TODO probably need a thing to specify the units
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

    void spawn(Entity& entity, vec2 pos) {
        spawn_fn(entity, pos);
        num_spawned++;
    }

    [[nodiscard]] bool validate(Entity& entity, vec2 pos) {
        return validation_spawn_fn ? validation_spawn_fn(entity, pos) : true;
    }

    [[nodiscard]] bool prevent_dupes() const {
        return prevent_duplicate_spawns;
    }

    auto& set_spawn_sound(const std::string& str) {
        spawn_sound = str;
        return *this;
    }
    [[nodiscard]] bool has_spawn_sound() const { return !spawn_sound.empty(); }
    [[nodiscard]] const std::string& get_spawn_sound() const {
        return spawn_sound;
    }

   private:
    bool prevent_duplicate_spawns = false;
    int max_spawned = 0;
    float spread = 0;

    int num_spawned = 0;
    float countdown = 0;
    SpawnFn spawn_fn;
    ValidationSpawnFn validation_spawn_fn;

    std::string spawn_sound;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // We likely dont need to serialize anything because it should be
        // all server side info

        // TODO add macro to only show these for debug builds
        // Debug only

        s.value4b(num_spawned);
        s.value4b(max_spawned);
    }
};
