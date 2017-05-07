#pragma once
#include <type_traits>
#include <memory>
#include "util.hpp"

namespace pal {
namespace traits {

/**
 * Pointers that can be converted to a weak pointer concept for tracking
 * purpose must implement the to_weak() function in order to make use of
 * ADL to convert that type and make it usable
 */

template<typename T>
std::weak_ptr<T> to_weak(std::weak_ptr<T> w) {
    return w;
}

template<typename T>
std::weak_ptr<T> to_weak(std::shared_ptr<T> s) {
    return s;
}

namespace detail {

template <typename T, typename = void>
struct is_weak_ptr : std::false_type {};

template <typename T>
struct is_weak_ptr<T, traits::void_t<decltype(std::declval<T>().expired()),
                                     decltype(std::declval<T>().lock()),
                                     decltype(std::declval<T>().reset())>>
    : std::true_type {};

template <typename T, typename = void>
struct is_weak_ptr_compatible : std::false_type {};

template <typename T>
struct is_weak_ptr_compatible<T, traits::void_t<decltype(to_weak(std::declval<T>()))>>
    : std::true_type {};

} // namespace detail

/// get the class type of a callable (no_class if not a class type)
template <typename P>
constexpr bool is_weak_ptr_v = detail::is_weak_ptr<P>::value;

template <typename P>
constexpr bool is_weak_ptr_compatible_v = detail::is_weak_ptr_compatible<P>::value;

} // namespace traits
} // namespace pal

