
#pragma once

#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
    struct Config {
        // TODO i dont think this is read more than on init()
        // need to find downstream users
        float round_length = 100.f;

        int max_num_orders = 1;
        float patience_multiplier = 1.f;
        float customer_spawn_multiplier = 1.f;
        int num_store_spawns = 5;

        float piss_timer = 2.5f;
        int bladder_size = 1;

        // TODO get_speed_for_entity

        bool has_city_multiplier = false;
        float cost_multiplier = 1.f;

        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value4b(round_length);
            s.value4b(max_num_orders);
            s.value4b(patience_multiplier);
            s.value4b(customer_spawn_multiplier);
            s.value4b(num_store_spawns);

            s.value4b(piss_timer);
            s.value4b(bladder_size);

            s.value1b(has_city_multiplier);
            s.value4b(cost_multiplier);
        }
    } config;

    virtual ~IsRoundSettingsManager() {}

    [[nodiscard]] float round_length() const { return config.round_length; }
    [[nodiscard]] int max_num_orders() const { return config.max_num_orders; }
    [[nodiscard]] float patience_multiplier() const {
        return config.patience_multiplier;
    }
    [[nodiscard]] float customer_spawn_multiplier() const {
        return config.customer_spawn_multiplier;
    }
    [[nodiscard]] int num_store_spawns() const {
        return config.num_store_spawns;
    }
    [[nodiscard]] float piss_timer() const { return config.piss_timer; }
    [[nodiscard]] int bladder_size() const { return config.bladder_size; }

    [[nodiscard]] bool has_city_multiplier() const {
        return config.has_city_multiplier;
    }
    [[nodiscard]] float cost_multiplier() const {
        return config.cost_multiplier;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(config);
    }
};
