#include "base_component.h"

#include "../entity.h"

void BaseComponent::attach_parent(Entity* p) {
    parent.reset(p);
    onAttach();
}

template<class Archive>
void BaseComponent::serialize(Archive& archive) {
    archive(parent);
}
