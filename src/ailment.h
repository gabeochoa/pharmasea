#pragma once

#include <cmath>
#include <set>
#include <string>

// modifies person behavior, applies debuffs/buffs
// hide from players, but for dev porpoises(haha) having a blip near the person
// could be useful for levelbuilding can have multiple ailments / can use this
// structure to have slowed/fast characters for example. Ex: ailment speed
// modifier stumbling/shaking? visual effect modifier(?) infectivity(?) gross -
// other persons avoid a radius around them

struct Symptom {
    struct Config {
        enum class Freq {
            Never = 0,
            Often = 1,  // 1 per minute
            //
            Count,
        } frequency = Freq::Never;

        enum class Strength {
            None,
            Low,
            Medium,
            High,
            //
            Count,
        } strength = Strength::None;

        bool operator<(const Config& sym) const {
            return (frequency < sym.frequency) ||
                   ((frequency == sym.frequency) && (strength < sym.strength));
        }

        template<typename T>
        T inc_int(T t) {
            return static_cast<T>((static_cast<int>(t) + 1) %
                                  (static_cast<int>(T::Count)));
        }

        template<typename T>
        T dec_int(T t) {
            return static_cast<T>((static_cast<int>(t) + 1) %
                                  (static_cast<int>(T::Count)));
        }

        Config& operator++() {
            frequency = inc_int<Freq>(frequency);
            strength = inc_int<Strength>(strength);
            return *this;
        }

        Config& operator--() {
            frequency = dec_int<Freq>(frequency);
            strength = dec_int<Strength>(strength);
            return *this;
        }

    } config;

    bool operator<(const Symptom& sym) const { return config < sym.config; }

    Symptom() {}
    explicit Symptom(const Config& opt) : config(opt) {}

    void worsen() { ++config; }
};

struct Sneeze : public Symptom {};
struct Vomit : public Symptom {};
struct Irritable : public Symptom {};
struct Sweaty : public Symptom {};
struct Bowels : public Symptom {};
struct Hallucination : public Symptom {};
struct Sleepy : public Symptom {};

//

struct Ailment {
    float speed_multiplier() {
        float value = [this]() {
            switch (config.speed) {
                case Speed::Slow:
                    return 0.1f;
                case Speed::Okay:
                    return 0.8f;
                case Speed::Default:
                    return 1.f;
                case Speed::Fast:
                    return 1.5f;
            }
        }();
        for (int i = 0; i < num_comorbids; i++) {
            value *= comorbids[i].speed_multiplier();
        }
        return clamp(overall() * value);
    }

    float stagger() {
        float value = [this]() {
            switch (config.stagger) {
                case Stagger::None:
                    return 0.0f;
                case Stagger::Little:
                    return 0.5f;
                case Stagger::Some:
                    return 0.8f;
                case Stagger::Lots:
                    return 1.0f;
            }
        }();

        for (int i = 0; i < num_comorbids; i++) {
            value *= comorbids[i].stagger();
        }

        float clamped = clamp(overall() * value);
        float mult = stagger_dir ? 1.5 : 0.5;
        stagger_dir = !stagger_dir;
        return mult * clamped;
    }

    // ensures the overall multiplier isnt over 10x strength
    float overall() { return clamp(overall_mult(), 0.1f, 10.f); }

   protected:
    // affects all others, bigger than 1.0 is stronger [0.1, 10]
    virtual float overall_mult() { return 1.0; }

   private:
    bool stagger_dir = false;
    virtual float clamp(float val, float mn = 0.1f, float mx = 1.f) {
        return fmaxf(mn, fminf(mx, val));
    }

   public:
    enum class Type {
        None,
        Test,
        Hooked,
        Insomnia,
    };

    enum class Speed {
        Slow,
        Okay,
        Default,
        Fast,
    };

    enum class Stagger {
        None,
        Little,
        Some,
        Lots,
    };

    struct Config {
        std::string_view name = "ailment";
        Type type = Type::None;
        Speed speed = Speed::Default;
        Stagger stagger = Stagger::None;
    } config;

    std::set<Symptom> symptoms;

    // Note: btw idk why i didnt use a vector

    // These are other ailments that can be caused by this ailment
    Ailment* comorbids = nullptr;
    int num_comorbids = 0;

    explicit Ailment(const Config& opt) : config(opt) {}
    virtual ~Ailment() {}
};

struct TEST_MAX_AIL : public Ailment {
    TEST_MAX_AIL()
        : Ailment({
              .name = "test_max_ailment",
              .type = Ailment::Type::Test,
              .speed = Ailment::Speed::Slow,
              .stagger = Ailment::Stagger::Lots,
          }) {}
};

struct Insomnia : public Ailment {
    Insomnia()
        : Ailment({
              .name = "Insomnia!",
              .type = Ailment::Type::Insomnia,
              .speed = Ailment::Speed::Slow,
              .stagger = Ailment::Stagger::None,
          }) {
        symptoms.insert(Irritable());
        symptoms.insert(Sleepy());
    }
};

struct Hooked : public Ailment {
    Hooked()
        : Ailment({
              .name = "Hooked!",
              .type = Ailment::Type::Hooked,
              .speed = Ailment::Speed::Default,
              .stagger = Ailment::Stagger::Lots,
          }) {
        symptoms.insert(Irritable());
        symptoms.insert(Sweaty());
        symptoms.insert(Hallucination());

        num_comorbids = 1;
        comorbids = new Ailment[1]{Insomnia()};
    }

    ~Hooked() { delete[] comorbids; }
};
