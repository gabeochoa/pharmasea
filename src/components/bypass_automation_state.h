#pragma once

#include "base_component.h"

struct BypassAutomationState : public BaseComponent {
    int configured_rounds = 0;
    int remaining_rounds = 0;
    bool exit_on_complete = false;
    bool recording_enabled = false;
    bool bypass_enabled = false;
    bool initialized = false;
    bool completed = false;
    // Per-flow state (replaces static locals in helper)
    float startup_delay = 0.0f;
    float host_role_set_time = -1.0f;
    bool play_clicked = false;
    bool username_saved = false;
    bool host_clicked = false;
    bool start_clicked = false;
    bool lobby_w_held = false;
    float lobby_entry_time = -1.0f;
    bool ingame_w_held = false;
    float ingame_entry_time = -1.0f;

    void configure(int rounds, bool exit_flag, bool recording_flag) {
        configured_rounds = rounds;
        remaining_rounds = rounds;
        exit_on_complete = exit_flag;
        recording_enabled = recording_flag;
        bypass_enabled = true;
        initialized = true;
        completed = false;
    }

    void mark_round_complete() {
        if (remaining_rounds > 0) {
            remaining_rounds--;
        }
        if (remaining_rounds <= 0) {
            completed = true;
            bypass_enabled = false;
        }
    }
};
