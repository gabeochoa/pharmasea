
#pragma once

//
#include "../entity_type.h"
#include "base_component.h"

struct Type : public BaseComponent {
    virtual ~Type() {}
    Type() {}

    explicit Type(EntityType t) : type(t) {}

    EntityType type = EntityType::Unknown;

    const std::string_view name() const {
        return magic_enum::enum_name<EntityType>(type);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(type);
    }
};
