#pragma once

#include "../components/is_pnumatic_pipe.h"
#include "../engine/assert.h"

namespace tests {

inline void test_is_pnumatic_pipe() {
    using PairingState = IsPnumaticPipe::PairingState;

    IsPnumaticPipe pipe;
    VALIDATE(pipe.pairing_state() == PairingState::Unpaired,
             "new pipe should be unpaired");
    VALIDATE(pipe.is_pairing_state(PairingState::Unpaired),
             "is_pairing_state should agree with pairing_state()");

    pipe.paired.id = 42;
    VALIDATE(pipe.pairing_state() == PairingState::PairedIdle,
             "paired pipe without transfer should be paired idle");

    pipe.recieving = true;
    VALIDATE(pipe.pairing_state() == PairingState::Transferring,
             "paired pipe receiving should be transferring");
    VALIDATE(pipe.is_pairing_state(PairingState::Transferring),
             "is_pairing_state should report transferring");
}

}  // namespace tests
