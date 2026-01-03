#pragma once

#include <string>
//
#include "base_component.h"

struct HasName : public BaseComponent {
    void update(const std::string& new_name) {
        _name = new_name;
        name_length = (int) _name.size();
    }

    const std::string& name() const { return _name; }

   private:
    int name_length = 1;
    std::string _name;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.name_length,                //
            self._name                       //
        );
    }
};
