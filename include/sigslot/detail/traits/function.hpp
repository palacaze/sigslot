#pragma once
#include <type_traits>
#include "typelist.hpp"

// traits that simplify function signature manipulation

namespace pal {
namespace traits {

struct no_class;

template <typename C, typename R, typename... A>
struct signature {
    using return_type = R;
    using args_type = typelist<A...>;
    using class_type = C;
};

namespace detail {

// remove class, const and volatile parts of a member function signature
template<typename T> struct remove_class {
    using type = T;
};

#define REMOVE_CLASS(q)                             \
template <typename C, typename R, typename... A>    \
struct remove_class<R(C::*)(A...) q> {              \
    using type = signature<C, R, A...>; };

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

template<typename... T>
struct get_sig_impl<signature<T...>> {
    using type = signature<T...>;
};

template <typename R, typename... A>
struct get_sig_impl<R(A...)> {
    using type = signature<no_class, R, A...>;
};

template <typename R, typename... A>
struct get_sig_impl<R(*)(A...)> {
    using type = signature<no_class, R, A...>;
};

template<typename F>
struct get_sig {
    using type = typename get_sig_impl<
        remove_class_t<
            std::decay_t<F>>>::type;
};

} // namespace detail

/// get the decomposed form (class, return and arguments) of a callable signature
template <typename F>
using sig_t = typename detail::get_sig<F>::type;

/// get the arguments list of a callable
template <typename F>
using args_t = typename sig_t<F>::args_type;

/// get the return type of a callable
template <typename F>
using return_t = typename sig_t<F>::return_type;

/// get the class type of a callable (no_class if not a class type)
template <typename F>
using class_t = typename sig_t<F>::class_type;

} // namespace traits
} // namespace pal

