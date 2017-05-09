#pragma once
#include <type_traits>
#include <utility>
#include <memory>

namespace pal {
namespace traits {

/// represent a list of types
template <typename...>
struct typelist {};

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

// tools
namespace detail {

template <class...>
struct voider { using type = void; };

// void_t from c++17
template <class...T>
using void_t = typename detail::voider<T...>::type;


template <typename, typename, typename = void, typename = void>
struct is_callable : std::false_type {};

template <typename F, typename P, typename... T>
struct is_callable<F, P, typelist<T...>, void_t<decltype(((*std::declval<P>()).*std::declval<F>())(std::declval<T>()...))>> : std::true_type {};

template <typename F, typename... T>
struct is_callable<F, typelist<T...>, void_t<decltype(std::declval<F>()(std::declval<T>()...))>> : std::true_type {};


template <typename T, typename = void>
struct is_weak_ptr : std::false_type {};

template <typename T>
struct is_weak_ptr<T, void_t<decltype(std::declval<T>().expired()),
                             decltype(std::declval<T>().lock()),
                             decltype(std::declval<T>().reset())>>
    : std::true_type {};

template <typename T, typename = void>
struct is_weak_ptr_compatible : std::false_type {};

template <typename T>
struct is_weak_ptr_compatible<T, void_t<decltype(to_weak(std::declval<T>()))>>
    : std::true_type {};

} // namespace detail

/// get the class type of a callable (no_class if not a class type)
template <typename P>
constexpr bool is_weak_ptr_v = detail::is_weak_ptr<P>::value;

/// determine if a pointer is convertible into a "weak" pointer
template <typename P>
constexpr bool is_weak_ptr_compatible_v = detail::is_weak_ptr_compatible<P>::value;

/// determine if a type T (Callale or Pmf) is callable with supplied arguments in L
template <typename L, typename... T>
constexpr bool is_callable_v = detail::is_callable<T..., L>::value;

} // namespace traits
} // namespace pal

