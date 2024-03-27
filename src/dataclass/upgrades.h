
#pragma once

#include <map>
#include <string>

#include "../components/is_progression_manager.h"
#include "configdata.h"
#include "settings.h"
#include "upgrade_class.h"

struct UpgradeImpl {
    UpgradeClass type;
    std::string name;
    std::string icon_name;
    std::string flavor_text;
    std::string description;
    std::function<Mods(const ConfigData&, const IsProgressionManager&, int)>
        onHourMods = nullptr;
    std::function<Actions(const ConfigData&, const IsProgressionManager&, int)>
        onHourActions = nullptr;
    std::function<void(ConfigData&, IsProgressionManager&)> onUnlock = nullptr;
    std::function<bool(const ConfigData&, const IsProgressionManager&)>
        meetsPrereqs = nullptr;
};

std::shared_ptr<UpgradeImpl> make_upgrade(UpgradeClass uc);