
#include <map>
#include <string>

#include "configdata.h"
#include "settings.h"

enum struct UpgradeClass { UnlockToilet };

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
        case UpgradeClass::UnlockToilet:
            ptr = new UpgradeImpl{
                .type = UpgradeClass::UnlockToilet,
                .name = "Gotta go",
                .icon_name = "gotta_go",
                .flavor_text = "Drinking has its consequences.",
                .description =
                    "(Customers will order twice, but will need to go to the "
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
    }

    if (ptr == nullptr) {
        log_error("Tried to fetch upgrade of type {} but didnt find anything",
                  magic_enum::enum_name<UpgradeClass>(uc));
    }
    return std::shared_ptr<UpgradeImpl>(ptr);
}
