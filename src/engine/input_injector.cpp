
#include "input_injector.h"

namespace input_injector {

namespace {
    struct PendingClick {
        bool has_pending = false;
        vec2 pos;
    };
    static PendingClick pending_click;
}

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
    
    // Don't release yet - we'll release it in ui::end() after buttons are checked
    // This allows active_if_mouse_inside to see leftDown=true and set widget as active,
    // then is_mouse_click will see leftDown=false (after we release) and detect the click
}

}  // namespace input_injector

