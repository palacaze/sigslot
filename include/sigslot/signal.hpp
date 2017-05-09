#pragma once
#include <memory>
#include <utility>
#include <mutex>
#include <atomic>
#include "detail/slot.hpp"
#include "detail/mutex.hpp"
#include "detail/util.hpp"
#include "connection.hpp"

namespace pal {

/**
 * signal_base is an implementation of the observer pattern, through the use
 * of an emitting object and slots that are connected to the signal and called
 * with supplied arguments when a signal is emitted.
 *
 * pal::signal_base is the general implementation, whose locking policy must be
 * set in order to decide thread safety guarantees. pal::signal and pal::signal_st
 * are partial specializations for multi-threaded and single-threaded use.
 *
 * It does not allow slots to return a value.
 *
 * @tparam Lockable a lock type to decide the lock policy
 * @tparam T... the argument types of the emitting and slots functions.
 */
template <typename Lockable, typename... T>
class signal_base {
    using lock_type = std::unique_lock<Lockable>;
    using slot_ptr = detail::slot_ptr<T...>;
    using arg_list = traits::typelist<T...>;

public:
    signal_base() noexcept : m_block(false) {}
    ~signal_base() {
        disconnect_all();
    }

    signal_base(const signal_base&) = delete;
    signal_base & operator=(const signal_base&) = delete;

    signal_base(signal_base && o)
        : m_block{o.m_block.load()}
    {
        lock_type lock(o.m_mutex);
        std::swap(m_slots, o.m_slots);
    }

    signal_base & operator=(signal_base && o) {
        lock_type lock1(m_mutex, std::defer_lock);
        lock_type lock2(o.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        std::swap(m_slots, o.m_slots);
        m_block.store(o.m_block.exchange(m_block.load()));
        return *this;
    }

    /**
     * Emit a signal
     *
     * Effect: All non blocked and connected slot functions will be called
     *         with supplied arguments.
     * Safety: With proper locking (see pal::signal), emission can happen from
     *         multiple threads simulateously. The guarantees only apply to the
     *         signal object, it does not cover thread safety of potentially
     *         shared state used in slot functions.
     *
     * @param a... arguments to emit
     */
    template <typename... A>
    void operator()(A && ... a) {
        lock_type lock(m_mutex);
        slot_ptr prev = nullptr;
        slot_ptr curr = m_slots;

        while (curr) {
            // call non blocked, non connected slots
            if (curr->connected()) {
                if (!m_block && !curr->blocked())
                    curr->operator()(std::forward<A>(a)...);
                prev = curr;
                curr = curr->next;
            }
            // remove slots marked as disconnected
            else {
                if (prev)
                    prev->next = curr->next;
                curr = curr->next;
            }
        }
    }

    /**
     * Connect a callable of compatible arguments
     *
     * Effect: Creates a new slot, with will call the supplied callable for
     *         every subsequent signal emission.
     * Safety: Thread-safety depends on locking policy.
     *
     * @param c a callable
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable,
              std::enable_if_t<traits::is_callable_v<arg_list, Callable>>* = nullptr>
    connection connect(Callable && c) {
        using slot_t = detail::slot<Callable, arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Callable>(c));
        add_slot(s);
        return connection(s);
    }

    /**
     * Overload of connect for pointers over member functions
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr,
              std::enable_if_t<traits::is_callable_v<arg_list, Pmf, Ptr> &&
                               !traits::is_weak_ptr_compatible_v<Ptr>>* = nullptr>
    connection connect(Pmf && pmf, Ptr ptr) {
        auto f = [=, pmf=std::forward<Pmf>(pmf)](auto && ...a) {
            (ptr->*pmf)(std::forward<decltype(a)>(a)...);
        };

        using slot_t = detail::slot<decltype(f), arg_list>;
        auto s = std::make_shared<slot_t>(std::move(f));
        add_slot(s);
        return connection(s);
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection
     *
     * Ptr must be convertible to an object following a loose form of weak pointer
     * concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a pointer over member function and a
     * trackable pointer of that class.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a suppied object.
     *
     * @param pmf a pointer over member function
     * @param ptr a trackable object pointer
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr,
              std::enable_if_t<!traits::is_callable_v<arg_list, Pmf> &&
                               traits::is_weak_ptr_compatible_v<Ptr>>* = nullptr>
    connection connect(Pmf && pmf, Ptr ptr) {
        using traits::to_weak;
        auto w = to_weak(ptr);

        auto f = [=, pmf=std::forward<Pmf>(pmf)](auto && ...a) {
            auto sp = w.lock();
            if (sp)
                ((*sp).*pmf)(std::forward<decltype(a)>(a)...);
        };

        using slot_t = detail::slot_tracked<decltype(f), decltype(w), arg_list>;
        auto s = std::make_shared<slot_t>(std::move(f), w);
        add_slot(s);
        return connection(s);
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection
     *
     * Trackable must be convertible to an object following a loose form of weak
     * pointer concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a standalone callable and unrelated trackable
     * object.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a suppied object.
     *
     * @param c a callable
     * @param ptr a trackable object pointer
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable, typename Trackable,
              std::enable_if_t<traits::is_callable_v<arg_list, Callable> &&
                               traits::is_weak_ptr_compatible_v<Trackable>>* = nullptr>
    connection connect(Callable && c, Trackable ptr) {
        using traits::to_weak;
        auto w = to_weak(ptr);
        using slot_t = detail::slot_tracked<Callable, decltype(w), arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Callable>(c), w);
        add_slot(s);
        return connection(s);
    }

    /**
     * Creates a connection whose duration is tied to the return object
     * Use the same semantics as connect
     */
    template <typename... CallArgs>
    scoped_connection connect_scoped(CallArgs && ...args) {
        return connect(std::forward<CallArgs>(args)...);
    }

    /**
     * Disconnects all the slots
     * Safety: Thread safety depends on locking policy
     */
    void disconnect_all() {
        lock_type lock(m_mutex);
        clear();
    }

    /**
     * Blocks signal emission
     * Safety: thread safe
     */
    void block() noexcept {
        m_block.store(true);
    }

    /**
     * Unblocks signal emission
     * Safety: thread safe
     */
    void unblock() noexcept {
        m_block.store(false);
    }

    /**
     * Tests blocking state of signal emission
     */
    bool blocked() const noexcept {
        return m_block.load();
    }

private:
    template <typename S>
    void add_slot(S &s) {
        lock_type lock(m_mutex);
        s->next = m_slots;
        m_slots = s;
    }

    void clear() {
        m_slots.reset();
    }

private:
    slot_ptr m_slots;
    Lockable m_mutex;
    std::atomic<bool> m_block;
};

/**
 * Specialization of signal_base to be used in single threaded contexts.
 * Slot connection, disconnection and signal emission are not thread-safe.
 * The performance improvement over the thread-safe variant is not impressive,
 * so this is not very useful.
 */
template <typename... T>
using signal_st = signal_base<detail::null_mutex, T...>;

/**
 * Specialization of signal_base to be used in multi-threaded contexts.
 * Slot connection, disconnection and signal emission are thread-safe.
 */
template <typename... T>
using signal = signal_base<std::mutex, T...>;

} // namespace pal

