#include "extern_templates.h"

// Explicit template instantiations to reduce compilation time
// These are instantiated once here instead of in every file that uses them

template bool Entity::has<Transform>();
template bool Entity::has<ModelRenderer>();
template bool Entity::has<CanHoldItem>();
template bool Entity::has<HasName>();
template bool Entity::has<SimpleColoredBoxRenderer>();

template bool Entity::is_missing<Transform>();
template bool Entity::is_missing<ModelRenderer>();
template bool Entity::is_missing<CanHoldItem>();
template bool Entity::is_missing<HasName>();
template bool Entity::is_missing<SimpleColoredBoxRenderer>();

template Transform& Entity::get<Transform>();
template ModelRenderer& Entity::get<ModelRenderer>();
template CanHoldItem& Entity::get<CanHoldItem>();
template HasName& Entity::get<HasName>();
template SimpleColoredBoxRenderer& Entity::get<SimpleColoredBoxRenderer>();

template Transform& Entity::addComponent<Transform>();
template ModelRenderer& Entity::addComponent<ModelRenderer>();
template CanHoldItem& Entity::addComponent<CanHoldItem>();
template HasName& Entity::addComponent<HasName>();
template SimpleColoredBoxRenderer& Entity::addComponent<SimpleColoredBoxRenderer>();
