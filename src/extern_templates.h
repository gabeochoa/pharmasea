#pragma once

#include "afterhours/src/core/entity.h"
#include "components/all_components.h"

// Extern template declarations to reduce template instantiation across multiple files
// This prevents duplicate instantiation of Entity methods for component types

extern template bool Entity::has<Transform>();
extern template bool Entity::has<ModelRenderer>();
extern template bool Entity::has<CanHoldItem>();
extern template bool Entity::has<HasName>();
extern template bool Entity::has<SimpleColoredBoxRenderer>();

extern template bool Entity::is_missing<Transform>();
extern template bool Entity::is_missing<ModelRenderer>();
extern template bool Entity::is_missing<CanHoldItem>();
extern template bool Entity::is_missing<HasName>();
extern template bool Entity::is_missing<SimpleColoredBoxRenderer>();

extern template Transform& Entity::get<Transform>();
extern template ModelRenderer& Entity::get<ModelRenderer>();
extern template CanHoldItem& Entity::get<CanHoldItem>();
extern template HasName& Entity::get<HasName>();
extern template SimpleColoredBoxRenderer& Entity::get<SimpleColoredBoxRenderer>();

extern template Transform& Entity::addComponent<Transform>();
extern template ModelRenderer& Entity::addComponent<ModelRenderer>();
extern template CanHoldItem& Entity::addComponent<CanHoldItem>();
extern template HasName& Entity::addComponent<HasName>();
extern template SimpleColoredBoxRenderer& Entity::addComponent<SimpleColoredBoxRenderer>();
