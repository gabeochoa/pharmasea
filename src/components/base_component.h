
#pragma once

#include <array>
#include <bitset>
#include <map>
#include <memory>

#include "../entity.h"
#include "../globals.h"
#include "../vendor_include.h"

struct BaseComponent {
    BaseComponent() {}
    BaseComponent(BaseComponent&) {}
    BaseComponent(BaseComponent&&) = default;

    virtual void onAttach() {}

    virtual ~BaseComponent() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S&) {}
};
