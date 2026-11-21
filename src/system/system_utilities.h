#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../entities/entity_helper.h"

namespace system_utils {

// For planning systems - runs when bar is closed
inline bool should_run_planning_system() {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        return hastimer.is_phase(HasDayNightTimer::Phase::Planning);
    } catch (...) {
        return false;
    }
}

// For inround systems - runs when bar is open
inline bool should_run_inround_system() {
    if (!GameState::get().is_game_like()) return false;
    try {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
        return hastimer.is_phase(HasDayNightTimer::Phase::InRound);
    } catch (...) {
        return false;
    }
}

}  // namespace system_utils
