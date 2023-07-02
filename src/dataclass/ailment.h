#pragma once

#include <array>
#include <cmath>
#include <set>
#include <string>

#include "../external_include.h"

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
            int val = static_cast<int>(t);
            int max = static_cast<int>(T::Count);
            if (val != max) val++;
            return static_cast<T>(val);
        }

        template<typename T>
        T dec_int(T t) {
            int val = static_cast<int>(t);
            if (val != 0) val--;
            return static_cast<T>(val);
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

       private:
        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value4b(frequency);
            s.value4b(strength);
        }

    } config;

    bool operator<(const Symptom& sym) const { return config < sym.config; }

    // TODO we only really need this today because below we dont want to specify
    // things as itll take time to balance correctly
    Symptom() {}
    explicit Symptom(const Config& opt) : config(opt) {}

    void worsen() { ++config; }
    void heal() { --config; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(config);
    }
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
                    return 0.5f;
                case Speed::Okay:
                    return 0.9f;
                case Speed::Default:
                    return 1.f;
                case Speed::Fast:
                    return 1.5f;
            }
            return 1.f;
        }();
        for (int i = 0; i < num_comorbids; i++) {
            value *= comorbids[i]->speed_multiplier();
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
            return 0.f;
        }();

        for (int i = 0; i < num_comorbids; i++) {
            value *= comorbids[i]->stagger();
        }

        float clamped = clamp(overall() * value);
        float mult = (stagger_dir % 5 == 0) ? 10.f : 0.f;
        stagger_dir = (stagger_dir + 1) % 100;
        return mult * clamped;
    }

    // ensures the overall multiplier isnt over 10x strength
    float overall() { return clamp(overall_mult(), 0.25f, 10.f); }

   protected:
    // affects all others, bigger than 1.0 is stronger [0.1, 10]
    float overall_mult() { return 1.0; }

   private:
    int stagger_dir = 0;
    float clamp(float val, float mn = 0.1f, float mx = 1.f) {
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
        std::string name = "ailment";
        Type type = Type::None;
        Speed speed = Speed::Default;
        Stagger stagger = Stagger::None;

       private:
        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            // TODO name constant
            s.text1b(name, 20);
            s.value4b(type);
            s.value4b(speed);
            s.value4b(stagger);
        }
    } config;

    // TODO make 10 a constant
    std::array<Symptom, 10> symptoms;

    // Note: btw idk why i didnt use a vector

    // These are other ailments that are caused by this ailment
    // TODO make 10 a constant
    std::array<std::shared_ptr<Ailment>, 10> comorbids;
    int num_comorbids = 0;

    Ailment() {}
    explicit Ailment(const Config& opt) : config(opt) {}
    ~Ailment() {}

    std::string icon_name() const {
        switch (config.type) {
            case Type::None:
                return "jug";
            case Type::Insomnia:
                return "sleepy";
            default:
            case Type::Test:
            case Type::Hooked:
                return "face";
        }
    }

    static Ailment* make_max_ailment() {
        return new Ailment({
            .name = "test_max_ailment",
            .type = Ailment::Type::Test,
            .speed = Ailment::Speed::Slow,
            .stagger = Ailment::Stagger::Lots,
        });
    }

    static Ailment* make_insomnia() {
        auto ailment = new Ailment({
            .name = "Insomnia",
            .type = Ailment::Type::Insomnia,
            .speed = Ailment::Speed::Okay,
            .stagger = Ailment::Stagger::None,
        });
        ailment->symptoms[0] = (Irritable());
        ailment->symptoms[1] = (Sleepy());
        return ailment;
    }

    static Ailment* make_hooked() {
        auto ailment = new Ailment({
            .name = "Hooked!",
            .type = Ailment::Type::Hooked,
            .speed = Ailment::Speed::Default,
            .stagger = Ailment::Stagger::Lots,
        });
        ailment->symptoms[0] = (Irritable());
        ailment->symptoms[1] = (Sweaty());
        ailment->symptoms[2] = (Hallucination());

        ailment->num_comorbids = 1;
        ailment->comorbids[0].reset(make_insomnia());
        return ailment;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(config);

        s.value4b(num_comorbids);
        s.container(comorbids);
        s.container(symptoms);
    }
};

template<typename S>
void serialize(S& s, std::shared_ptr<Ailment>& data) {
    s.ext(data, bitsery::ext::StdSmartPtr{});
}
