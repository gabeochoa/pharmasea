#pragma once

#include "external_include.h"
#include "uuid.h"

namespace ui {

template<typename T>
struct State {
   private:
    T value = T();

   public:
    bool changed_since = false;

    State() {}
    State(T val) : value(val) {}
    State(const State<T>& s) : value(s.value) {}
    State<T>& operator=(const State<T>& other) {
        this->value = other.value;
        changed_since = true;
        return *this;
    }
    operator T() { return value; }
    T& asT() { return value; }
    T& get() { return value; }
    void set(const T& v) {
        value = v;
        changed_since = true;
    }

    // TODO how can we support += and -=?
    // is there a simple way to unfold the types,
    // tried and seeing nullptr for checkboxstate
};

// Base statetype for any UI component
struct UIState {
    virtual ~UIState() {}
};

struct ToggleState : public UIState {
    State<bool> on;
};

struct DropdownState : public ToggleState {};
struct CheckboxState : public ToggleState {};

struct ButtonListState : public UIState {
    State<int> selected;
    State<bool> hasFocus;
};

struct SliderState : public UIState {
    State<float> value;
};

struct TextfieldState : public UIState {
    State<std::string> buffer;
    State<int> cursorBlinkTime;
    State<bool> showCursor;
};

////////////////
////////////////
////////////////
////////////////

struct StateManager {
    std::map<uuid, std::shared_ptr<UIState>> states;

    void addState(const uuid id, const std::shared_ptr<UIState>& state) {
        states[id] = state;
    }

    template<typename T>
    std::shared_ptr<T> getAndCreateIfNone(const uuid id) {
        if (states.find(id) == states.end()) {
            addState(id, std::make_shared<T>());
        }
        return get_as<T>(id);
    }

    std::shared_ptr<UIState> get(const uuid id) { return states[id]; }

    template<typename T>
    std::shared_ptr<T> get_as(const uuid id) {
        try {
            return dynamic_pointer_cast<T>(states.at(id));
        } catch (std::exception) {
            return nullptr;
        }
    }
};

////////////////
////////////////
////////////////
////////////////

}  // namespace ui
