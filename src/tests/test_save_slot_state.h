#pragma once

#include "../engine/assert.h"
#include "../save_game/save_game.h"

namespace tests {

inline void test_save_slot_state() {
    using namespace save_game;

    SlotInfo info;
    VALIDATE(info.state() == SlotState::Empty,
             "missing slot should be empty");

    info.exists = true;
    VALIDATE(info.state() == SlotState::Corrupt,
             "existing slot without a header should be corrupt");

    info.header = SaveGameHeader{};
    VALIDATE(info.state() == SlotState::Valid,
             "existing slot with a current header should be valid");

    info.header->save_version = 999;
    VALIDATE(info.state() == SlotState::VersionMismatch,
             "slot with a foreign save version should mismatch");
}

}  // namespace tests
