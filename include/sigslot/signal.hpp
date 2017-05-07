#pragma once
#include <memory>
#include <utility>
#include <mutex>
#include <atomic>
#include "detail/slot.hpp"
#include "detail/traits/pointer.hpp"
#include "connection.hpp"

namespace pal {

/**
 * A signal is an implementation of the observer pattern, through the use of
 * an emitting object and slots that are connected to the signal and called
 * with supplied arguments when a signal is emitted.
 *
 * pal::signal is a simple, thead-safe implementation, that does not allow
 * slots to return a value.
 *
 * @tparam T... the argument types of the emitting and slots functions.
 */
template <typename ...T>
class signal {
    using lock_type = std::unique_lock<std::mutex>;
    using slot_ptr = detail::slot_ptr<T...>;

public:
    signal() noexcept : m_block(false) {}
    ~signal() {
        disconnect_all();
    }

    signal(const signal&) = delete;
    signal & operator=(const signal&) = delete;

    signal(signal && o)
        : m_block{o.m_block.load()}
    {
        lock_type lock(o.m_mutex);
        std::swap(m_slots, o.m_slots);
    }

    signal & operator=(signal && o) {
        lock_type lock1(m_mutex, std::defer_lock);
        lock_type lock2(o.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        std::swap(m_slots, o.m_slots);
        m_block.store(o.m_block.exchange(m_block.load()));
        return *this;
    }

    /**
     * emit a signal
     *
     * effect: all non blocked and connected slot functions will be called
     *         with supplied arguments.
     * safety: emission can happen from multiple threads. The guarantees
     *         only apply to the signal object, it does not cover thread
     *         safety of potentially shared state used in slot functions.
     *
     * @param a arguments to emit
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
     * connect a callable of compatible arguments
     *
     * effect: creates a new slot, with will call the supplied callable for
     *         every subsequent signal emission.
     * safety: thread safe
     *
     * @param c a callable
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    connection connect(Callable && c) {
        auto s = detail::make_slot(std::forward<Callable>(c));
        add_slot(s);
        return connection(s);
    }

    /**
     * overload of connect for pointers over member functions
     *
     * @param c a pointer over member function
     * @param p an object pointer
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable, typename Ptr,
              typename TC = traits::class_t<Callable>,
              typename TP = std::remove_cv_t<std::remove_pointer_t<Ptr>>,
              std::enable_if_t<!traits::is_weak_ptr_compatible_v<Ptr> &&
                               std::is_same<TP, TC>::value>* = nullptr>
    connection connect(Callable && c, Ptr p) {
        auto s = detail::make_slot(std::forward<Callable>(c), p);
        add_slot(s);
        return connection(s);
    }

    /**
     * overload of connect for lifetime object tracking
     *
     * This overload allows automatic disconnection when the tracked object is
     * destroyed, it covers two cases:
     * - standalone callable and an object whose lifetime is trackable
     * - a pointer over member function and a trackable pointer for the suitable class
     *
     * In both cases the Ptr type must be convertible to an object following a
     * loose form of weak pointer concept, by implementing the ADL-detected
     * conversion function to_weak().
     *
     * Note: only weak reference are stored, a slot does not extend the lifetime
     * of a suppied object.
     */
    template <typename Callable, typename Ptr,
              std::enable_if_t<traits::is_weak_ptr_compatible_v<Ptr>>* = nullptr>
    connection connect(Callable && c, Ptr p) {
        using traits::to_weak;
        auto s = detail::make_slot_tracked(std::forward<Callable>(c), to_weak(p));
        add_slot(s);
        return connection(s);
    }

    /**
     * Create a connection whose duration is tied to the return object
     *
     * The three connection_scoped overloads do the same operations as three
     * connect ones, except for the returned scoped_connection object, which
     * is a RAII object that disconnects the slot upon destruction.
     */
    template <typename Callable>
    scoped_connection connect_scoped(Callable && c) {
        auto s = detail::make_slot(std::forward<Callable>(c));
        add_slot(s);
        return scoped_connection(s);
    }

    template <typename Callable, typename Ptr,
              typename TC = traits::class_t<Callable>,
              typename TP = std::remove_cv_t<std::remove_pointer_t<Ptr>>,
              std::enable_if_t<!traits::is_weak_ptr_compatible_v<Ptr> &&
                               std::is_same<TP, TC>::value>* = nullptr>
    scoped_connection connect_scoped(Callable && c, Ptr p) {
        auto s = detail::make_slot(std::forward<Callable>(c), p);
        add_slot(s);
        return connection_scoped(s);
    }

    template <typename Callable, typename Ptr,
              std::enable_if_t<traits::is_weak_ptr_compatible_v<Ptr>>* = nullptr>
    scoped_connection connect_scoped(Callable && c, Ptr p) {
        using traits::to_weak;
        auto s = detail::make_slot_tracked(std::forward<Callable>(c), to_weak(p));
        add_slot(s);
        return scoped_connection(s);
    }

    /**
     * disconnects all the slots
     * safety: thread safe
     */
    void disconnect_all() {
        lock_type lock(m_mutex);
        clear();
    }

    /**
     * blocks signal emission
     * safety: thread safe
     */
    void block() noexcept {
        m_block.store(true);
    }

    /**
     * unblocks signal emission
     * safety: thread safe
     */
    void unblock() noexcept {
        m_block.store(false);
    }

    /**
     * tests blocking state of signal emission
     */
    bool blocked() const noexcept {
        m_block.load();
    }

private:
    template <typename S>
    void add_slot(S &s) {
        using slot_args = typename S::element_type::base_types;
        using sig_args = traits::typelist<T...>;
        static_assert(std::is_same<slot_args, sig_args>::value,
            "Attempt to connect a slot to a signal of incompatible argument types");

        lock_type lock(m_mutex);
        s->next = m_slots;
        m_slots = s;
    }

    void clear() {
        m_slots.reset();
    }

private:
    slot_ptr m_slots;
    std::mutex m_mutex;
    std::atomic<bool> m_block;
};

} // namespace pal

