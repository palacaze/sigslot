#pragma once

namespace pal {
namespace traits {
namespace detail {

template <class...>
struct voider { using type = void; };

} // namespace detail

// void_t from c++17
template <class...T>
using void_t = typename detail::voider<T...>::type;

} // namespace traits
} // namespace pal

