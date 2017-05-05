#pragma once
#include <type_traits>
#include "typelist.hpp"

// traits that simplify function signature manipulation

namespace pal {
namespace traits {
namespace detail {

// remove class, const and volatile parts of a member function signature
template<typename T> struct remove_class {
    using type = T;
};

#define REMOVE_CLASS(q)                                           \
template <typename C, typename R, typename... A>                  \
struct remove_class<R(C::*)(A...) q> { using type = R(A...); };

REMOVE_CLASS()
REMOVE_CLASS(const)
REMOVE_CLASS(volatile)
REMOVE_CLASS(const volatile)

template <typename F>
using remove_class_t = typename remove_class<F>::type;

// strip callable signature to a common baseline, able to handle function
// objets, static functions, lambdas and member function uniformly
template<typename T>
struct get_sig_impl {
    using call_op = decltype(&std::remove_reference<T>::type::operator());
    using type = remove_class_t<std::decay_t<call_op>>;
};

template <typename R, typename... A>
struct get_sig_impl<R(A...)> {
    using type = R(A...);
};

template <typename R, typename... A>
struct get_sig_impl<R(*)(A...)> {
    using type = R(A...);
};

template<typename F>
struct get_sig {
    using type = typename get_sig_impl<
        remove_class_t<
            std::decay_t<F>>>::type;
};

// get function arguments
template <typename...>
struct get_args {};

template <typename R, typename... A>
struct get_args<R(A...)> {
    using type = typelist<A...>;
};

// get function return type
template <typename...>
struct get_ret {};

template <typename R, typename... A>
struct get_ret<R(A...)> {
    using type = R;
};

} // namespace detail

/// get the "canonical" form of a callable signature
template <typename F>
using sig_t = typename detail::get_sig<F>::type;

/// get the arguments list of a callable
template <typename F>
using args_t = typename detail::get_args<sig_t<F>>::type;

/// get the return type of a callable
template <typename F>
using return_t = typename detail::get_ret<sig_t<F>>::type;

} // namespace traits
} // namespace pal

