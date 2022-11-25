#pragma once

#include <cmath>
// modifies person behavior, applies debuffs/buffs
// hide from players, but for dev porpoises(haha) having a blip near the person
// could be useful for levelbuilding can have multiple ailments / can use this
// structure to have slowed/fast characters for example. Ex: ailment speed
// modifier stumbling/shaking? visual effect modifier(?) infectivity(?) gross -
// other persons avoid a radius around them

// Names
// Hooked! - addiction debuff
//

struct Ailment {
    struct Options {
        // NOTE: 1.0 is "normal" even though for some it might not make sense
        float speed = 1.f;
        float gross = 1.f;
        float shaky = 1.f;
    } options;

    Ailment(Options opt) : options(opt) {}

    float speed_multiplier() { return clamp(overall() * options.speed); }
    // float grossness() { return 1 - clamp(overall() * options.gross); }
    // float shakyness() { return 1 - clamp(overall() * options.shaky); }

    // ensures the overall multiplier isnt over 10x strength
    float overall() { return clamp(overall_mult(), 0.1f, 10.f); }

   protected:
    // affects all others, bigger than 1.0 is stronger [0.1, 10]
    virtual float overall_mult() { return 1.0; }

   private:
    virtual float clamp(float val, float mn = 0.1f, float mx = 1.f) {
        return fmaxf(mn, fminf(mx, val));
    }
};

struct TEST_MAX_AIL : public Ailment {
    TEST_MAX_AIL()
        : Ailment({
              .speed = 0.f,
              // .gross = 0.f,
              // .shaky = 0.f,
          }) {}
};
