
#pragma once

template<class SourceContainer>
class ContainerConverter {
    const SourceContainer& s_;

   public:
    explicit ContainerConverter(const SourceContainer& s) : s_(s) {}

    // Dont make explicit
    template<class TargetContainer>
    operator TargetContainer() const {
        return TargetContainer(s_.begin(), s_.end());
    }
};

template<class C>
// This function takes a string, so that you as the developer can write why you
// need to do this please dont ignore thx
ContainerConverter<C> container_cast(const C& c, const char*) {
    return ContainerConverter<C>(c);
}
