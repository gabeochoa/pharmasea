

#pragma once

#include "../furniture.h"
#include "base_component.h"

struct CanHoldFurniture : public BaseComponent {
    virtual ~CanHoldFurniture() {}

    [[nodiscard]] bool empty() const { return held_furniture == nullptr; }
    [[nodiscard]] bool is_holding_furniture() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_furniture);
    }

    void update(std::shared_ptr<Furniture> furniture) {
        held_furniture = furniture;
    }

    [[nodiscard]] std::shared_ptr<Furniture> furniture() {
        return held_furniture;
    }

   private:
    std::shared_ptr<Furniture> held_furniture = nullptr;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.ext(held_furniture, bitsery::ext::StdSmartPtr{});
    }
};
