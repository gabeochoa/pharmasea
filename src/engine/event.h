
#pragma once

#include "../external_include.h"
#include <concepts>
#include "gamepad_axis_with_dir.h"

enum class EventType {
    None = 0,
    WindowClose,
    WindowResize,
    WindowFullscreen,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseButtonUp,
    MouseButtonDown,
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

template<typename T>
concept DispatchableEvent =
    std::derived_from<T, Event> &&
    requires { { T::getStaticType() } -> std::same_as<EventType>; };

struct EventDispatcher {
    explicit EventDispatcher(Event& e) : event(e) {}
    Event& event;

    template<DispatchableEvent T>
    bool dispatch(const std::function<bool(T&)>& func) {
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

struct WindowFullscreenEvent : public Event {
    bool on;
    explicit WindowFullscreenEvent(bool enable) : on(enable) {}

    MACRO_EVENT_TYPE(WindowFullscreen)
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

namespace Mouse {
enum MouseCode {
    // From glfw3.h
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Button6 = 6,
    Button7 = 7,

    ButtonLast = Button7,
    ButtonLeft = Button0,
    ButtonRight = Button1,
    ButtonMiddle = Button2
};

class MouseMovedEvent : public Event {
   public:
    explicit MouseMovedEvent(const float x, const float y)
        : mouseX(x), mouseY(y) {}

    float x() const { return mouseX; }
    float y() const { return mouseY; }

    std::string toString() const override {
        return fmt::format("MouseMovedEvent: {},{}", mouseX, mouseY);
    }

    MACRO_EVENT_TYPE(MouseMoved)
    MACRO_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)
   private:
    float mouseX, mouseY;
};

class MouseScrolledEvent : public Event {
   public:
    MouseScrolledEvent(const float xOffset, const float yOffset)
        : XOffset(xOffset), YOffset(yOffset) {}

    float GetXOffset() const { return XOffset; }
    float GetYOffset() const { return YOffset; }

    std::string toString() const override {
        return fmt::format("MouseScrolledEvent: {},{}", GetXOffset(),
                           GetYOffset());
    }

    MACRO_EVENT_TYPE(MouseScrolled)
    MACRO_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput)
   private:
    float XOffset, YOffset;
};

struct MouseButtonEvent : public Event {
   public:
    MouseCode GetMouseButton() const { return button; }

    MACRO_EVENT_CATEGORY(EventCategoryMouse | EventCategoryInput |
                         EventCategoryMouseButton)
   protected:
    MouseButtonEvent(const MouseCode b) : button(b) {}

    MouseCode button;
};

struct MouseButtonPressedEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonPressedEvent(const int b)
        : MouseButtonEvent(static_cast<MouseCode>(b)) {}
    explicit MouseButtonPressedEvent(const MouseCode b) : MouseButtonEvent(b) {}

    std::string toString() const override {
        return fmt::format("MouseButtonPressedEvent: {}", button);
    }

    MACRO_EVENT_TYPE(MouseButtonPressed)
};

struct MouseButtonReleasedEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonReleasedEvent(const int b)
        : MouseButtonEvent(static_cast<MouseCode>(b)) {}
    explicit MouseButtonReleasedEvent(const MouseCode b)
        : MouseButtonEvent(b) {}

    std::string toString() const override {
        return fmt::format("MouseButtonReleasedEvent: ", button);
    }

    MACRO_EVENT_TYPE(MouseButtonReleased)
};

struct MouseButtonUpEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonUpEvent(const int b)
        : MouseButtonEvent(static_cast<MouseCode>(b)) {}
    explicit MouseButtonUpEvent(const MouseCode b) : MouseButtonEvent(b) {}

    std::string toString() const override {
        return fmt::format("MouseButtonUpEvent: {}", button);
    }

    MACRO_EVENT_TYPE(MouseButtonUp)
};

struct MouseButtonDownEvent : public MouseButtonEvent {
   public:
    explicit MouseButtonDownEvent(const int b)
        : MouseButtonEvent(static_cast<MouseCode>(b)) {}
    explicit MouseButtonDownEvent(const MouseCode b) : MouseButtonEvent(b) {}

    std::string toString() const override {
        return fmt::format("MouseButtonDownEvent: ", button);
    }

    MACRO_EVENT_TYPE(MouseButtonDown)
};
}  // namespace Mouse
