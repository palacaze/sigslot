#pragma once
#include <memory>
#include <utility>
#include <atomic>
#include "traits/function.hpp"

namespace pal {
namespace detail {

/* slot_state holds slot type independant state, to be used to interact with
 * slots indirectly through connection and scoped_connection objects.
 */
class slot_state {
public:
    constexpr slot_state() noexcept
        : m_connected(true),
          m_blocked(false) {}

    virtual ~slot_state() = default;

    bool connected() const noexcept { return m_connected; }
    bool disconnect() noexcept { return m_connected.exchange(false); }

    bool blocked() const noexcept { return m_blocked.load(); }
    void block()   noexcept { m_blocked.store(true); }
    void unblock() noexcept { m_blocked.store(false); }

private:
    std::atomic<bool> m_connected;
    std::atomic<bool> m_blocked;
};


template <typename...>
class slot_base;

template <typename... T>
using slot_ptr = std::shared_ptr<slot_base<T...>>;

/* A base class for slot objects. This base type only depends on slot argument
 * types, it will be used as an element in an intrusive singly-linked list of
 * slots, hence the public next member.
 */
template <typename... Args>
class slot_base : public slot_state {
public:
    using base_types = traits::typelist<Args...>;

    virtual ~slot_base() = default;

    // method effectively responible for calling the "slot" function with
    // supplied arguments whenever emission happens.
    virtual void call_slot(Args && ...) = 0;

    template <typename... U>
    void operator()(U && ...u) {
        if (slot_state::connected() && !slot_state::blocked())
            call_slot(std::forward<U>(u)...);
    }

    slot_ptr<Args...> next;
};

/*
 * A slot object holds state information and a callable to to be called
 * whenever the function call operator of its slot_base base class is called.
 */
template <typename Func, typename... Args>
class slot : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot(F && f) : func{std::forward<F>(f)} {}

    virtual void call_slot(Args && ...args) override {
        func(std::forward<decltype(args)>(args)...);
    }

private:
    Func func;
};

/*
 * An implementation of a slot that tracks the life of a supplied object
 * through a weak pointer in order to automatically disconnect the slot
 * on said object destruction.
 */
template <typename Func, typename WeakPtr, typename... Args>
class slot_tracked : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_tracked(F && f, P && p)
        : func{std::forward<F>(f)},
          ptr{std::forward<P>(p)}
    {}

    virtual void call_slot(Args && ...args) override {
        if (! slot_state::connected())
            return;
        if (ptr.expired())
            slot_state::disconnect();
        else
            func(std::forward<decltype(args)>(args)...);
    }

private:
    Func func;
    WeakPtr ptr;
};

template <typename Func>
auto make_slot(Func && func) {
    using slot_t = traits::apply_t<
        slot,
        traits::add_first_t<
            traits::args_t<Func>,
            Func>>;
    return std::make_shared<slot_t>(std::forward<Func>(func));
}

template <typename Func, typename Obj>
auto make_slot(Func func, Obj *obj) {
    auto f = [=, f=std::forward<Func>(func)](auto && ...a) {
        (obj->*func)(std::forward<decltype(a)>(a)...);
    };

    using slot_t = traits::apply_t<
        slot,
        traits::add_first_t<
            traits::args_t<Func>,
            decltype(f)>>;

    return std::make_shared<slot_t>(std::move(f));
}

namespace detail {
template <typename F, typename P>
constexpr bool is_weak_pmf =
    std::is_same<traits::class_t<F>, typename P::element_type>::value;
} // namespace detail

template <typename Func, typename Ptr,
          std::enable_if_t<not detail::is_weak_pmf<Func, Ptr>>* = nullptr>
auto make_slot_tracked(Func && func, Ptr ptr) {
    using slot_t = traits::apply_t<
        slot_tracked,
        traits::concat_t<
            traits::typelist<Func, Ptr>,
            traits::args_t<Func>>>;

    return std::make_shared<slot_t>(std::forward<Func>(func), ptr);
}

template <typename Func, typename Ptr,
          std::enable_if_t<detail::is_weak_pmf<Func, Ptr>>* = nullptr>
auto make_slot_tracked(Func && func, Ptr ptr) {
    auto f = [=, f=std::forward<Func>(func)](auto && ...a) {
        auto sptr = ptr.lock();
        if (sptr)
            ((*sptr).*func)(std::forward<decltype(a)>(a)...);
    };

    using slot_t = traits::apply_t<
        slot_tracked,
        traits::concat_t<
            traits::typelist<decltype(f), Ptr>,
            traits::args_t<Func>>>;

    return std::make_shared<slot_t>(std::move(f), ptr);
}

} // namespace detail
} // namespace pal

