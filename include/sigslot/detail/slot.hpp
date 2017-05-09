#pragma once
#include <memory>
#include <utility>
#include <atomic>
#include "util.hpp"

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
    virtual void call_slot(Args...) = 0;

    template <typename... U>
    void operator()(U && ...u) {
        if (slot_state::connected() && !slot_state::blocked())
            call_slot(std::forward<U>(u)...);
    }

    slot_ptr<Args...> next;
};

template <typename, typename...> class slot {};

/*
 * A slot object holds state information and a callable to to be called
 * whenever the function call operator of its slot_base base class is called.
 */
template <typename Func, typename... Args>
class slot<Func, traits::typelist<Args...>> : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot(F && f) : func{std::forward<F>(f)} {}

    virtual void call_slot(Args ...args) override {
        func(args...);
    }

private:
    Func func;
};

template <typename, typename, typename...> class slot_tracked {};

/*
 * An implementation of a slot that tracks the life of a supplied object
 * through a weak pointer in order to automatically disconnect the slot
 * on said object destruction.
 */
template <typename Func, typename WeakPtr, typename... Args>
class slot_tracked<Func, WeakPtr, traits::typelist<Args...>> : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_tracked(F && f, P && p)
        : func{std::forward<F>(f)},
          ptr{std::forward<P>(p)}
    {}

    virtual void call_slot(Args ...args) override {
        if (! slot_state::connected())
            return;
        if (ptr.expired())
            slot_state::disconnect();
        else
            func(args...);
    }

private:
    Func func;
    WeakPtr ptr;
};

} // namespace detail
} // namespace pal

