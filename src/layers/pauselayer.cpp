
#include "pauselayer.h"

#include "../components/is_round_settings_manager.h"
#include "../entities/entity_query.h"
#include "../network/api.h"

void BasePauseLayer::reset_network() { network::reset_connections(); }

void BasePauseLayer::draw_upgrades(Rectangle window, Rectangle rect) {
    OptEntity sophie =
        EntityQuery().whereHasComponent<IsRoundSettingsManager>().gen_first();
    if (!sophie) return;

    const IsRoundSettingsManager& irsm = sophie->get<IsRoundSettingsManager>();
    const ConfigData& config = irsm.config;

    if (config.num_unique_upgrades_unlocked() == 0) return;

    if (config.num_unique_upgrades_unlocked() > 100) {
        log_warn("More upgrades than we can display");
    }

    auto upgrades = rect::bpad(window, 90);

    std::vector<Rectangle> rects;
    upgrades = rect::lpad(upgrades, 10);
    upgrades = rect::rpad(upgrades, 90);
    auto rows = rect::hsplit<10>(upgrades, 20);
    for (auto r : rows) {
        auto cols = rect::vsplit<10>(r, 20);
        for (auto c : cols) {
            rects.push_back(c);
        }
    }

    int i = 0;

    // TODO when an upgrade is unlocked multiple times, display a 2x or
    // whatever on the icon
    std::shared_ptr<UpgradeImpl> hovered_upgrade = nullptr;

    config.for_each_unlocked([&](UpgradeClass uc) {
        if (i > (int) rects.size()) return bitset_utils::ForEachFlow::Break;

        Widget icon = Widget{rects[i]};

        auto upgradeImpl = make_upgrade(uc);

        if (irsm.is_upgrade_active(upgradeImpl->type)) {
            div(icon, ui::theme::Usage::Primary);
        }

        image(icon, upgradeImpl->icon_name);
        if (hoverable(icon)) {
            hovered_upgrade = upgradeImpl;
        }
        i++;
        return bitset_utils::ForEachFlow::NormalFlow;
    });

    if (hovered_upgrade) {
        div(rect, ui::theme::Usage::Background);

        const auto [header, rest] = rect::hsplit<2>(rect);

        const auto icon = rect::rpad(header, 80);
        const auto name = rect::lpad(header, 10);
        image(icon, hovered_upgrade->icon_name);
        text(name, hovered_upgrade->name);

        const auto [flavor, desc] = rect::hsplit<2>(rest);
        text(flavor, hovered_upgrade->flavor_text);
        text(desc, hovered_upgrade->description);
    }
}
