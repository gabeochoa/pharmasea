#pragma once

#include "base_component.h"

struct HasLoadSaveMode : public BaseComponent {
    enum class Mode : int { Load = 0, Delete = 1 };

    [[nodiscard]] Mode mode() const { return current_mode; }

    [[nodiscard]] bool is_delete_mode() const {
        return current_mode == Mode::Delete;
    }

    bool toggle() {
        current_mode = is_delete_mode() ? Mode::Load : Mode::Delete;
        return is_delete_mode();
    }

   private:
    Mode current_mode = Mode::Load;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.current_mode                   //
        );
    }
};
