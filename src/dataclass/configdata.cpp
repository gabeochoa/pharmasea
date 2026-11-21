
#include "configdata.h"

#include "upgrades.h"

void ConfigData::mark_upgrade_unlocked(const UpgradeClass& uc) {
    if (!bitset_utils::test(unlocked_upgrades, uc)) {
        bitset_utils::set(unlocked_upgrades, uc);
        return;
    }

    // if it was already unlocked is this something reusable?
    if (!upgrade_class_is_reusable(uc)) {
        log_warn("You are unlocking {} again but its not marked reusable...",
                 magic_enum::enum_name<UpgradeClass>(uc));
        return;
    }

    // it was reusable, so increment the count
    if (!reusable_counts.contains(uc)) {
        reusable_counts[uc] = 0;
    }
    reusable_counts[uc]++;
}

void ConfigData::for_each_unlocked(
    const std::function<bitset_utils::ForEachFlow(UpgradeClass)>& cb) const {
    bitset_utils::for_each_enabled_bit(unlocked_upgrades, [&](size_t index) {
        UpgradeClass uc = magic_enum::enum_value<UpgradeClass>(index);
        return cb(uc);
    });
}

size_t ConfigData::count_missing_prereqs(const IsProgressionManager& ipm) {
    const ConfigData& config = *this;
    size_t missing_prereq = 0;

    magic_enum::enum_for_each<UpgradeClass>([&](auto val) {
        constexpr UpgradeClass upgrade = val;

        // skip it if its already unlocked
        if (has_upgrade_unlocked(upgrade)) return;

        auto impl = make_upgrade(upgrade);
        bool meets = impl->meetsPrereqs(config, ipm);
        if (!meets) {
            missing_prereq++;
        }
    });
    return missing_prereq;
}

std::vector<std::shared_ptr<UpgradeImpl>> ConfigData::get_possible_upgrades(
    const IsProgressionManager& ipm) {
    const ConfigData& config = *this;

    size_t total_upgrades = magic_enum::enum_count<UpgradeClass>();
    size_t reusables = ReusableUpgrades.size();
    size_t missing_prereq = count_missing_prereqs(ipm);

    // How many non reusable/non unlocked upgrades do we have left to see?
    size_t remaining = total_upgrades                    //
                       - num_unique_upgrades_unlocked()  //
                       - reusables                       //
                       - missing_prereq;

    // if theres less than 2 remaining, then allow reusable ones
    bool allow_reusable = remaining < 2;

    std::vector<std::shared_ptr<UpgradeImpl>> possible_upgrades;

    const auto unlockable = [&](const UpgradeClass& uc) {
        // First time? we good
        bool already_unlocked = has_upgrade_unlocked(uc);
        if (!already_unlocked) return true;

        // if its already unlocked, is this a reusable one?
        bool is_reusable = upgrade_class_is_reusable(uc);
        if (!is_reusable) return false;

        // If its already unlocked and reusable, we can only reuse
        // when all the other upgrades are done,

        // only allow reusables when theres no other unique ones left
        return allow_reusable;
    };

    magic_enum::enum_for_each<UpgradeClass>([&](auto val) {
        constexpr UpgradeClass upgrade = val;

        if (!unlockable(upgrade)) return;

        auto impl = make_upgrade(upgrade);
        bool meets = impl->meetsPrereqs(config, ipm);
        if (meets) {
            possible_upgrades.push_back(impl);
        }
    });

    return possible_upgrades;
}
