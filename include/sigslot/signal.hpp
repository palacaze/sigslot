#pragma once
#include <type_traits>
#include <memory>
#include <utility>
#include <mutex>
#include <atomic>

namespace sigslot {

namespace trait {

/// represent a list of types
template <typename...> struct typelist {};

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
struct is_callable<F, P, typelist<T...>,
        void_t<decltype(((*std::declval<P>()).*std::declval<F>())(std::declval<T>()...))>>
    : std::true_type {};

template <typename F, typename... T>
struct is_callable<F, typelist<T...>,
        void_t<decltype(std::declval<F>()(std::declval<T>()...))>>
    : std::true_type {};


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
    : is_weak_ptr<decltype(to_weak(std::declval<T>()))> {};

} // namespace detail

/// determine if a pointer is convertible into a "weak" pointer
template <typename P>
constexpr bool is_weak_ptr_compatible_v = detail::is_weak_ptr_compatible<std::decay_t<P>>::value;

/// determine if a type T (Callable or Pmf) is callable with supplied arguments in L
template <typename L, typename... T>
constexpr bool is_callable_v = detail::is_callable<T..., L>::value;

} // namespace trait


namespace detail {

/* slot_state holds slot type independent state, to be used to interact with
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

} // namespace detail

/**
 * connection_blocker is a RAII object that blocks a connection until destruction
 */
class connection_blocker {
public:
    connection_blocker() = default;
    ~connection_blocker() noexcept { release(); }

    connection_blocker(const connection_blocker &) = delete;
    connection_blocker & operator=(const connection_blocker &) = delete;

    connection_blocker(connection_blocker && o) noexcept
        : m_state{std::move(o.m_state)}
    {}

    connection_blocker & operator=(connection_blocker && o) noexcept {
        release();
        m_state.swap(o.m_state);
        return *this;
    }

private:
    friend class connection;
    connection_blocker(std::weak_ptr<detail::slot_state> s) noexcept
        : m_state{std::move(s)}
    {
        auto d = m_state.lock();
        if (d) d->block();
    }

    void release() noexcept {
        auto d = m_state.lock();
        if (d) d->unblock();
    }

private:
    std::weak_ptr<detail::slot_state> m_state;
};


/**
 * A connection object allows interaction with an ongoing slot connection
 *
 * It allows common actions such as connection blocking and disconnection.
 * Note that connection is not a RAII object, one does not need to hold one
 * such object to keep the signal-slot connection alive.
 */
class connection {
public:
    connection() = default;
    virtual ~connection() = default;

    connection(const connection &) noexcept = default;
    connection & operator=(const connection &) noexcept = default;
    connection(connection &&) noexcept = default;
    connection & operator=(connection &&) noexcept = default;

    bool valid() const noexcept {
        return !m_state.expired();
    }

    bool connected() const noexcept {
        const auto d = m_state.lock();
        return d && d->connected();
    }

    bool disconnect() noexcept {
        auto d = m_state.lock();
        return d && d->disconnect();
    }

    bool blocked() const noexcept {
        const auto d = m_state.lock();
        return d && d->blocked();
    }

    void block() noexcept {
        auto d = m_state.lock();
        if(d)
            d->block();
    }

    void unblock() noexcept {
        auto d = m_state.lock();
        if(d)
            d->unblock();
    }

    connection_blocker blocker() const noexcept {
        return connection_blocker{m_state};
    }

protected:
    template <typename, typename...> friend class signal_base;
    connection(std::weak_ptr<detail::slot_state> s) noexcept
        : m_state{std::move(s)}
    {}

protected:
    std::weak_ptr<detail::slot_state> m_state;
};

/**
 * scoped_connection is a RAII version of connection
 * It disconnects the slot from the signal upon destruction.
 */
class scoped_connection : public connection {
public:
    scoped_connection() = default;
    ~scoped_connection() {
        disconnect();
    }

    scoped_connection(const connection &c) noexcept : connection(c) {}
    scoped_connection(connection &&c) noexcept : connection(std::move(c)) {}

    scoped_connection(const scoped_connection &) noexcept = delete;
    scoped_connection & operator=(const scoped_connection &) noexcept = delete;

    scoped_connection(scoped_connection && o) noexcept
        : connection{std::move(o.m_state)}
    {}

    scoped_connection & operator=(scoped_connection && o) noexcept {
        disconnect();
        m_state.swap(o.m_state);
        return *this;
    }

private:
    template <typename, typename...> friend class signal_base;
    scoped_connection(std::weak_ptr<detail::slot_state> s) noexcept
        : connection{std::move(s)}
    {}
};

namespace detail {

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
    using base_types = trait::typelist<Args...>;

    virtual ~slot_base() = default;

    // method effectively responsible for calling the "slot" function with
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
 * A slot object holds state information, and a callable to to be called
 * whenever the function call operator of its slot_base base class is called.
 */
template <typename Func, typename... Args>
class slot<Func, trait::typelist<Args...>> : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot(F && f) : func{std::forward<F>(f)} {}

    virtual void call_slot(Args ...args) override {
        func(args...);
    }

private:
    std::decay_t<Func> func;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Func, typename... Args>
class slot<Func, trait::typelist<connection&, Args...>> : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot(F && f) : func{std::forward<F>(f)} {}

    virtual void call_slot(Args ...args) override {
        func(conn, args...);
    }

    connection conn;

private:
    std::decay_t<Func> func;
};

/*
 * A slot object holds state information, an object and a pointer over member
 * function to be called whenever the function call operator of its slot_base
 * base class is called.
 */
template <typename Pmf, typename Ptr, typename... Args>
class slot<Pmf, Ptr, trait::typelist<Args...>> : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot(F && f, P && p)
        : pmf{std::forward<F>(f)},
          ptr{std::forward<P>(p)} {}

    virtual void call_slot(Args ...args) override {
        ((*ptr).*pmf)(args...);
    }

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<Ptr> ptr;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Pmf, typename Ptr, typename... Args>
class slot<Pmf, Ptr, trait::typelist<connection&, Args...>> : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot(F && f, P && p)
        : pmf{std::forward<F>(f)},
          ptr{std::forward<P>(p)} {}

    virtual void call_slot(Args ...args) override {
        ((*ptr).*pmf)(conn, args...);
    }

    connection conn;

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<Ptr> ptr;
};

template <typename, typename, typename...> class slot_tracked {};

/*
 * An implementation of a slot that tracks the life of a supplied object
 * through a weak pointer in order to automatically disconnect the slot
 * on said object destruction.
 */
template <typename Func, typename WeakPtr, typename... Args>
class slot_tracked<Func, WeakPtr, trait::typelist<Args...>> : public slot_base<Args...> {
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
    std::decay_t<Func> func;
    std::decay_t<WeakPtr> ptr;
};

template <typename, typename, typename...> class slot_pmf_tracked {};

/*
 * An implementation of a slot as a pointer over member function, that tracks
 * the life of a supplied object through a weak pointer in order to automatically
 * disconnect the slot on said object destruction.
 */
template <typename Pmf, typename WeakPtr, typename... Args>
class slot_pmf_tracked<Pmf, WeakPtr, trait::typelist<Args...>> : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_pmf_tracked(F && f, P && p)
        : pmf{std::forward<F>(f)},
          ptr{std::forward<P>(p)}
    {}

    virtual void call_slot(Args ...args) override {
        if (! slot_state::connected())
            return;
        auto sp = ptr.lock();
        if (!sp)
            slot_state::disconnect();
        else
            ((*sp).*pmf)(args...);
    }

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<WeakPtr> ptr;
};


// noop mutex for thread-unsafe use
struct null_mutex {
    null_mutex() = default;
    null_mutex(const null_mutex &) = delete;
    null_mutex operator=(const null_mutex &) = delete;
    null_mutex(null_mutex &&) = delete;
    null_mutex operator=(null_mutex &&) = delete;

    bool try_lock() { return true; }
    void lock() {}
    void unlock() {}
};

} // namespace detail


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

public:
    using arg_list = trait::typelist<T...>;
    using ext_arg_list = trait::typelist<connection&, T...>;

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
     *         multiple threads simultaneously. The guarantees only apply to the
     *         signal object, it does not cover thread safety of potentially
     *         shared state used in slot functions.
     *
     * @param a... arguments to emit
     */
    template <typename... A>
    void operator()(A && ... a) {
        lock_type lock(m_mutex);
        slot_ptr *prev = nullptr;
        slot_ptr *curr = m_slots ? &m_slots : nullptr;

        while (curr) {
            // call non blocked, non connected slots
            if ((*curr)->connected()) {
                if (!m_block && !(*curr)->blocked())
                    (*curr)->operator()(std::forward<A>(a)...);
                prev = curr;
                curr = (*curr)->next ? &((*curr)->next) : nullptr;
            }
            // remove slots marked as disconnected
            else {
                if (prev) {
                    (*prev)->next = (*curr)->next;
                    curr = (*prev)->next ? &((*prev)->next) : nullptr;
                }
                else
                    curr = (*curr)->next ? &((*curr)->next) : nullptr;
            }
        }
    }

    /**
     * Connect a callable of compatible arguments
     *
     * Effect: Creates and stores a new slot responsible for executing the
     *         supplied callable for every subsequent signal emission.
     * Safety: Thread-safety depends on locking policy.
     *
     * @param c a callable
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    std::enable_if_t<trait::is_callable_v<arg_list, Callable>, connection>
    connect(Callable && c) {
        using slot_t = detail::slot<Callable, arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Callable>(c));
        add_slot(s);
        return connection(s);
    }

    /**
     * Connect a callable with an additional connection argument
     *
     * The callable's first argument must be of type connection. This overload
     * the callable to manage it's own connection through this argument.
     *
     * @param c a callable
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    std::enable_if_t<trait::is_callable_v<ext_arg_list, Callable>, connection>
    connect_extended(Callable && c) {
        using slot_t = detail::slot<Callable, ext_arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Callable>(c));
        s->conn = connection(s);
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
    template <typename Pmf, typename Ptr>
    std::enable_if_t<trait::is_callable_v<arg_list, Pmf, Ptr> &&
                     !trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect(Pmf && pmf, Ptr && ptr) {
        using slot_t = detail::slot<Pmf, Ptr, arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr));
        add_slot(s);
        return connection(s);
    }

    /**
     * Overload  of connect for pointer over member functions and
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    std::enable_if_t<trait::is_callable_v<ext_arg_list, Pmf, Ptr> &&
                     !trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect_extended(Pmf && pmf, Ptr && ptr) {
        using slot_t = detail::slot<Pmf, Ptr, ext_arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr));
        s->conn = connection(s);
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
    template <typename Pmf, typename Ptr>
    std::enable_if_t<!trait::is_callable_v<arg_list, Pmf> &&
                     trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect(Pmf && pmf, Ptr && ptr) {
        using trait::to_weak;
        auto w = to_weak(std::forward<Ptr>(ptr));
        using slot_t = detail::slot_pmf_tracked<Pmf, decltype(w), arg_list>;
        auto s = std::make_shared<slot_t>(std::forward<Pmf>(pmf), w);
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
    template <typename Callable, typename Trackable>
    std::enable_if_t<trait::is_callable_v<arg_list, Callable> &&
                     trait::is_weak_ptr_compatible_v<Trackable>, connection>
    connect(Callable && c, Trackable && ptr) {
        using trait::to_weak;
        auto w = to_weak(std::forward<Trackable>(ptr));
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
 *
 * Beware of accidentally using recursive signal emission or cycles between
 * two or more signals in your code. Locking std::mutex more than once is
 * undefined behaviour, even if it "seems to work somehow". Use signal_r
 * instead for that use case.
 */
template <typename... T>
using signal = signal_base<std::mutex, T...>;

/**
 * Specialization of signal_base to be used in multi-threaded contexts, allowing
 * for recursive signal emission and emission cycles.
 * Slot connection, disconnection and signal emission are thread-safe.
 */
template <typename... T>
using signal_r = signal_base<std::recursive_mutex, T...>;

} // namespace sigslot

