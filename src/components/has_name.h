#pragma once

#include <string>
//
#include "base_component.h"

struct HasName : public BaseComponent {
    virtual ~HasName() {}

    void update(const std::string& new_name) {
        _name = new_name;
        name_length = (int) _name.size();
    }

    const std::string& name() const { return _name; }

   private:
    int name_length = 1;
    std::string _name;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                name_length, _name);
    }
};

CEREAL_REGISTER_TYPE(HasName);
