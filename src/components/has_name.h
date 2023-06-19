#pragma once

//
#include "base_component.h"

struct HasName : public BaseComponent {
    virtual ~HasName() {}

    void update(const std::string& new_name) {
        _name = new_name;
        name_length = (int) _name.size();
    }

    const std::string name() const { return _name; }

   private:
    int name_length = 1;
    std::string _name = "";

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(name_length);
        s.text1b(_name, name_length);
    }
};
