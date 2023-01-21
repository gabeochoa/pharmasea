#pragma once

//
#include "base_component.h"

struct HasName : public BaseComponent {
    int name_length = 1;
    std::string name = "";

    virtual ~HasName() {}

    void update(const std::string& new_name) {
        name = new_name;
        name_length = (int) name.size();
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(name_length);
        s.text1b(name, name_length);
    }
};
