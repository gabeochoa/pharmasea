#include "simulated_input.h"

namespace input_injector {

namespace {
struct PendingClick {
    bool has_pending = false;
    vec2 pos;
};
static PendingClick pending_click;

struct PendingKeyHold {
    bool is_holding = false;
    int keycode = 0;
    float remaining_time = 0.0f;
};
static PendingKeyHold pending_key_hold;

// Synthetic key states used by replay; does NOT generate repeat events.
static std::array<bool, 512> synthetic_keys{};
static std::array<int, 512> synthetic_press_count{};
// Delay consumption by one frame to avoid immediate same-frame consumers.
static std::array<int, 512> synthetic_press_delay{};
}  // namespace

void release_scheduled_click() {
    if (pending_click.has_pending && ui::focus::mouse_info.leftDown) {
        ui::focus::mouse_info.leftDown = false;
        Mouse::MouseButtonUpEvent up_event(Mouse::MouseCode::ButtonLeft);
        App::get().processEvent(up_event);
        pending_click.has_pending = false;
    }
}

void schedule_mouse_click_at(const Rectangle& rect) {
    vec2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
    pending_click.has_pending = true;
    pending_click.pos = center;
}

void inject_scheduled_click() {
    if (!pending_click.has_pending) return;

    ui::focus::mouse_info.pos = pending_click.pos;
    ui::focus::mouse_info.leftDown = true;
    Mouse::MouseButtonDownEvent down_event(Mouse::MouseCode::ButtonLeft);
    App::get().processEvent(down_event);
}

void hold_key_for_duration(int keycode, float duration) {
    pending_key_hold.is_holding = true;
    pending_key_hold.keycode = keycode;
    pending_key_hold.remaining_time = duration;
}

void set_key_down(int keycode) {
    if (keycode >= 0 && keycode < static_cast<int>(synthetic_keys.size())) {
        synthetic_keys[static_cast<size_t>(keycode)] = true;
        synthetic_press_count[static_cast<size_t>(keycode)]++;
        synthetic_press_delay[static_cast<size_t>(keycode)] = 1;
        if (keycode == raylib::KEY_SPACE) {
            log_info("injector: key {} down count={}", keycode,
                     synthetic_press_count[static_cast<size_t>(keycode)]);
        }
    }
}

void set_key_up(int keycode) {
    if (keycode >= 0 && keycode < static_cast<int>(synthetic_keys.size())) {
        synthetic_keys[static_cast<size_t>(keycode)] = false;
        if (keycode == raylib::KEY_SPACE) {
            log_info("injector: key {} up", keycode);
        }
    }
}

bool consume_synthetic_press(int keycode) {
    if (keycode < 0 || keycode >= static_cast<int>(synthetic_press_count.size())) {
        return false;
    }
    size_t idx = static_cast<size_t>(keycode);
    if (synthetic_press_count[idx] > 0) {
        if (synthetic_press_delay[idx] > 0) {
            synthetic_press_delay[idx]--;
            return false;
        }
        synthetic_press_count[idx]--;
        if (keycode == raylib::KEY_SPACE) {
            log_info("injector: consume key {} remaining={}", keycode,
                     synthetic_press_count[idx]);
        }
        return true;
    }
    return false;
}

void update_key_hold(float dt) {
    if (!pending_key_hold.is_holding) return;

    KeyPressedEvent event(pending_key_hold.keycode, 0);
    App::get().processEvent(event);

    pending_key_hold.remaining_time -= dt;
    if (pending_key_hold.remaining_time <= 0.0f) {
        pending_key_hold.is_holding = false;
    }
}

bool is_key_synthetically_down(int keycode) {
    if (keycode < 0 || keycode >= static_cast<int>(synthetic_keys.size())) {
        return false;
    }
    return synthetic_keys[static_cast<size_t>(keycode)];
}

}  // namespace input_injector
