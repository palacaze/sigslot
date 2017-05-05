#pragma once
#include <type_traits>

// traits to ease working with lists of types

namespace pal {
namespace traits {

/// represent a list of types
template <typename...>
struct typelist {};

namespace detail {

template <typename...>
struct concat_lists {};

template <typename... U, typename... T>
struct concat_lists<typelist<U...>, typelist<T...>> {
    using type = typelist<U..., T...>;
};

template <typename, typename...>
struct add_first {};

template <typename T, typename... A>
struct add_first<typelist<A...>, T> {
    using type = typelist<T, A...>;
};

template <typename, typename...>
struct add_last {};

template <typename T, typename... A>
struct add_last<typelist<A...>, T> {
    using type = typelist<A..., T>;
};

template <typename>
struct rem_first {};

template <>
struct rem_first<typelist<>> {
    using type = typelist<>;
};

template <typename T, typename... A>
struct rem_first<typelist<T, A...>> {
    using type = typelist<A...>;
};

template <typename, typename...>
struct rem_last {};

template <>
struct rem_last<typelist<>> {
    using type = typelist<>;
};

template <typename T>
struct rem_last<typelist<T>> {
    using type = typelist<>;
};

template <typename T, typename... A>
struct rem_last<typelist<T, A...>> {
    using type = typename concat_lists<
        typelist<T>,
        typename rem_last<typelist<A...>>::type
        >::type;
};

template <template <typename...> class Container, typename, typename...>
struct apply_types {};

template <template <typename...> class Container, typename... A>
struct apply_types<Container, typelist<A...>> {
    using type = Container<A...>;
};

template <template <typename> class, typename>
struct filter_types {};

template <template <typename> class Pred>
struct filter_types<Pred, typelist<>> {
    using type = typelist<>;
};

template <template <typename> class Pred, typename T, typename... R>
struct filter_types<Pred, typelist<T, R...>> {
    using type = typename concat_lists<
        std::conditional_t<
            Pred<T>::value,
            typelist<T>,
            typelist<>>,
        typename filter_types<Pred, typelist<R...>>::type
    >::type;
};

} // namespace detail


/// concatenate 2 typelists
template <typename L1, typename L2>
using concat_t = typename detail::concat_lists<L1, L2>::type;

/// add an element at the begining of a typelist
template <typename L, typename T>
using add_first_t = typename detail::add_first<L, T>::type;

/// remove the first element of a typelist
template <typename L>
using rem_first_t = typename detail::rem_first<L>::type;

/// add an element at the end of a typelist
template <typename L, typename T>
using add_last_t = typename detail::add_last<L, T>::type;

/// remove an element at the end of a typelist
template <typename L>
using rem_last_t = typename detail::rem_last<L>::type;

/// filter a typelist with a predicate
template <template <typename> class Pred, typename L>
using filter_t = typename detail::filter_types<Pred, L>::type;

/// apply the typelist to a type container
template <template <typename...> class C, typename L>
using apply_t = typename detail::apply_types<C, L>::type;

} // namespace traits
} // namespace pal

