
#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

template<class C>
using container_cast_begin_t = decltype(std::begin(std::declval<const C&>()));

template<class C>
using container_cast_end_t = decltype(std::end(std::declval<const C&>()));

template<class SourceContainer>
requires std::ranges::input_range<SourceContainer>
class ContainerConverter {
    const SourceContainer& s_;

   public:
    explicit ContainerConverter(const SourceContainer& s) : s_(s) {}

    // Dont make explicit
    template<class TargetContainer>
    requires std::constructible_from<TargetContainer,
                                     container_cast_begin_t<SourceContainer>,
                                     container_cast_end_t<SourceContainer>>
    operator TargetContainer() const {
        return TargetContainer(s_.begin(), s_.end());
    }
};

template<class C>
// This function takes a string, so that you as the developer can write why you
// need to do this please dont ignore thx
requires std::ranges::input_range<C>
ContainerConverter<C> container_cast(const C& c, const char*) {
    return ContainerConverter<C>(c);
}
