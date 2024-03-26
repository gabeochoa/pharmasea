
#pragma once

#include <map>
#include <string>

#include "configdata.h"
#include "settings.h"

enum struct UpgradeClass { LongerDay, UnlockToilet };

struct UpgradeImpl {
    UpgradeClass type;
    std::string name;
    std::string icon_name;
    std::string flavor_text;
    std::string description;
    std::function<void(ConfigData&)> onUnlock;
    std::function<bool(const ConfigData&)> meetsPrereqs;
};

static std::shared_ptr<UpgradeImpl> make_upgrade(UpgradeClass uc) {
    UpgradeImpl* ptr = nullptr;

    switch (uc) {
        case UpgradeClass::LongerDay:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Longer Day",
                .icon_name = "longer_day",
                .flavor_text = "Find an extra couple hours in the day.",
                .description = "(Makes the day twice as long)",
                .onUnlock =
                    [](ConfigData& config) {
                        {
                            int val = config.get<int>(ConfigKey::RoundLength);
                            config.set<int>(ConfigKey::RoundLength, val * 2);
                        }
                    },
                .meetsPrereqs = [](const ConfigData&) -> bool { return true; }};
            break;
        case UpgradeClass::UnlockToilet:
            ptr = new UpgradeImpl{
                .type = uc,
                .name = "Gotta go",
                .icon_name = "gotta_go",
                .flavor_text = "Drinking has its consequences.",
                .description =
                    "(Customers will order twice, but will need to go to "
                    "the "
                    "bathroom)",
                .onUnlock =
                    [](ConfigData& config) {
                        {
                            int val = config.get<int>(ConfigKey::MaxNumOrders);
                            config.set<int>(ConfigKey::MaxNumOrders, val * 2);
                        }
                        { config.set<bool>(ConfigKey::UnlockedToilet, true); }
                        {
                            // TODO add toilet to store spawn
                        } {
                            // TODO add toilet to required machines
                        }
                    },
                .meetsPrereqs = [](const ConfigData& config) -> bool {
                    return config.get<bool>(ConfigKey::UnlockedToilet) == false;
                }};
            break;
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }
    return std::shared_ptr<UpgradeImpl>(ptr);
}
