
#pragma once

#include <map>
#include <string>

#include "../components/is_progression_manager.h"
#include "configdata.h"
#include "upgrade_class.h"

struct UpgradeImpl {
    UpgradeClass type;
    std::string name;
    std::string icon_name;
    std::string flavor_text;
    std::string description;
    std::function<void(ConfigData&, IsProgressionManager&)> onUnlock;
    std::function<bool(const ConfigData&, const IsProgressionManager&)>
        meetsPrereqs;
};

std::shared_ptr<UpgradeImpl> make_upgrade(UpgradeClass uc);
