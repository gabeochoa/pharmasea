
#pragma once

#include "../external_include.h"
#include "gamepad_axis_with_dir.h"

enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    CharPressed,
    GamepadAxisMoved,
    GamepadButtonPressed,
};

#define BIT(x) (1 << x)
enum EventCategory {
    EventCategoryNone = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4),
    EventCategoryGamepad = BIT(5),
};

#define MACRO_EVENT_TYPE(type)                                   \
    static EventType getStaticType() { return EventType::type; } \
    virtual EventType getEventType() const override {            \
        return getStaticType();                                  \
    }                                                            \
    virtual const char* getName() const override { return #type; }

#define MACRO_EVENT_CATEGORY(cat) \
    virtual int getCategoryFlags() const override { return cat; }

struct Event {
    bool handled = false;

    friend struct EventDispatcher;

    virtual ~Event() {}

    virtual EventType getEventType() const = 0;
    virtual const char* getName() const = 0;
    virtual int getCategoryFlags() const = 0;
    virtual std::string toString() const { return getName(); }
    [[nodiscard]] bool isInCategory(EventCategory cat) const {
        return getCategoryFlags() & cat;
    }
};
struct EventDispatcher {
    explicit EventDispatcher(Event& e) : event(e) {}
    Event& event;

    template<typename T>
    bool dispatch(std::function<bool(T&)> func) {
        if (event.getEventType() == T::getStaticType()) {
            event.handled = func(*(T*) &event);
            return true;
        }
        return false;
    }
};

struct KeyEvent : public Event {
    explicit KeyEvent(int k) : keycode(k) {}
    int keycode;
    [[nodiscard]] int getKeyCode() const { return keycode; }

    MACRO_EVENT_CATEGORY(EventCategoryKeyboard | EventCategoryInput);
};

struct KeyPressedEvent : public KeyEvent {
    MACRO_EVENT_TYPE(KeyPressed)

    KeyPressedEvent(int k, int r) : KeyEvent(k), repeatCount(r) {}
    int repeatCount;
    [[nodiscard]] int getRepeatCount() const { return repeatCount; }
};

struct CharPressedEvent : public KeyEvent {
    MACRO_EVENT_TYPE(CharPressed)

    CharPressedEvent(int k, int r) : KeyEvent(k), repeatCount(r) {}
    int repeatCount;
    [[nodiscard]] int getRepeatCount() const { return repeatCount; }
};

struct WindowResizeEvent : public Event {
    unsigned int width, height;
    WindowResizeEvent(unsigned int w, unsigned int h) : width(w), height(h) {}

    // std::string toString() const override {
    // return fmt::format("WindowResizeEvent: {}, {}", Width, Height);
    // }

    MACRO_EVENT_TYPE(WindowResize)
    MACRO_EVENT_CATEGORY(EventCategoryApplication)
};

struct GamepadButtonPressedEvent : public Event {
    raylib::GamepadButton button;

    explicit GamepadButtonPressedEvent(raylib::GamepadButton butt)
        : button(butt) {}

    MACRO_EVENT_TYPE(GamepadButtonPressed)
    MACRO_EVENT_CATEGORY(EventCategoryGamepad | EventCategoryInput)
};

struct GamepadAxisMovedEvent : public Event {
    GamepadAxisWithDir data;

    explicit GamepadAxisMovedEvent(GamepadAxisWithDir info) : data(info) {}

    MACRO_EVENT_TYPE(GamepadAxisMoved)
    MACRO_EVENT_CATEGORY(EventCategoryGamepad | EventCategoryInput)
};
