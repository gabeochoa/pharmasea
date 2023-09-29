
#pragma once

#include <iostream>
//
#include "../entity_type.h"
#include "base_component.h"

struct DebugName : public BaseComponent {
    virtual ~DebugName() {}

    void update(EntityType t) { type = t; }

    [[nodiscard]] bool is_type(EntityType t) const { return type == t; }
    const EntityType& get_type() const { return type; }
    std::string_view name() const {
        return magic_enum::enum_name<EntityType>(type);
    }

   private:
    EntityType type;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(type);
    }
};

inline std::ostream& operator<<(std::ostream& os, const DebugName& dn) {
    os << dn.name();
    return os;
}
