
#include "is_progression_manager.h"

#include <utility>

#include "../dataclass/upgrades.h"
#include "../engine/unreachable.h"

TranslatableString IsProgressionManager::get_option_title(bool is_first) const {
    TranslatableString internal_ts =
        TODO_TRANSLATE("(invalid)", TodoReason::UserFacingError);
    switch (upgrade_type()) {
        case UpgradeType::None:
            return internal_ts;

        case UpgradeType::Drink:
            return TODO_TRANSLATE(std::string(magic_enum::enum_name<Drink>(
                                      is_first ? drinkOption1 : drinkOption2)),
                                  TodoReason::SubjectToChange);
        case UpgradeType::Upgrade:
            auto impl =
                make_upgrade(is_first ? upgradeOption1 : upgradeOption2);
            return impl->name;
    }
    unreachable();
    // return TranslatableString{""};
}

TranslatableString IsProgressionManager::get_option_subtitle(
    bool is_first) const {
    TranslatableString internal_ts =
        TODO_TRANSLATE("(invalid)", TodoReason::UserFacingError);
    switch (upgrade_type()) {
        case UpgradeType::None:
            return internal_ts;
        case UpgradeType::Drink:
            return TODO_TRANSLATE(std::string(magic_enum::enum_name<Drink>(
                                      is_first ? drinkOption1 : drinkOption2)),
                                  TodoReason::SubjectToChange);
        case UpgradeType::Upgrade:
            auto impl =
                make_upgrade(is_first ? upgradeOption1 : upgradeOption2);
            return impl->name;
    }
    unreachable();
    return TranslatableString{""};
}
