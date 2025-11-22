
#include "input_injector.h"

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

    // Set mouse position - done AFTER focus::begin() which resets it
    ui::focus::mouse_info.pos = pending_click.pos;

    // Set mouse down so active_if_mouse_inside will set widget as active
    // We keep it down during button checks, then release it at end of frame
    ui::focus::mouse_info.leftDown = true;
    Mouse::MouseButtonDownEvent down_event(Mouse::MouseCode::ButtonLeft);
    App::get().processEvent(down_event);

    // Don't release yet - we'll release it in ui::end() after buttons are
    // checked This allows active_if_mouse_inside to see leftDown=true and set
    // widget as active, then is_mouse_click will see leftDown=false (after we
    // release) and detect the click
}

void hold_key_for_duration(int keycode, float duration) {
    pending_key_hold.is_holding = true;
    pending_key_hold.keycode = keycode;
    pending_key_hold.remaining_time = duration;
}

void update_key_hold(float dt) {
    if (!pending_key_hold.is_holding) return;

    // Inject key press event each frame while holding
    KeyPressedEvent event(pending_key_hold.keycode, 0);
    App::get().processEvent(event);

    // Update remaining time
    pending_key_hold.remaining_time -= dt;
    if (pending_key_hold.remaining_time <= 0.0f) {
        pending_key_hold.is_holding = false;
    }
}

bool is_key_synthetically_down(int keycode) {
    return pending_key_hold.is_holding && pending_key_hold.keycode == keycode;
}

}  // namespace input_injector
