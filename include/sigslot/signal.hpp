#pragma once
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace sigslot {

namespace trait {

/// represent a list of types
template <typename...> struct typelist {};

/**
 * Pointers that can be converted to a weak pointer concept for tracking
 * purpose must implement the to_weak() function in order to make use of
 * ADL to convert that type and make it usable
 */

template <typename T>
std::weak_ptr<T> to_weak(std::weak_ptr<T> w) {
    return w;
}

template <typename T>
std::weak_ptr<T> to_weak(std::shared_ptr<T> s) {
    return s;
}

// tools
namespace detail {

template <typename...>
struct voider { using type = void; };

// void_t from c++17
template <typename...T>
using void_t = typename detail::voider<T...>::type;

template <typename, typename = void>
struct has_call_operator : std::false_type {};

template <typename F>
struct has_call_operator<F, void_t<decltype(&F::operator())>> : std::true_type {};

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

template <typename T>
constexpr bool is_weak_ptr_v = detail::is_weak_ptr<T>::value;

template <typename T>
constexpr bool has_call_operator_v = detail::has_call_operator<T>::value;

template <typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;

template <typename T>
constexpr bool is_func_v = std::is_function<T>::value;

template <typename T>
constexpr bool is_pmf_v = std::is_member_function_pointer<T>::value;

} // namespace trait

template <typename, typename...>
class signal_base;

namespace detail {

/*
 * The following callable_pointer and object_pointer series of templates are
 * used to circumvent the type-erasing that takes place in the slot_base
 * implementations. They do so by obtaining the address of the callables and
 * objects and casting them to a void type
 *
 * NOTE: I am not convinced there is no UB happening for pointers to member
 * functions, this is pretty funky.
 */

using data_ptr = const void*;
using call_pptr = data_ptr*;

template <typename T, typename = void>
struct callable_pointer {
    static void get_ptr(const T &, call_pptr p) {
        *p = nullptr;
    }
};

template <typename T>
struct callable_pointer<T, std::enable_if_t<trait::is_func_v<T>>> {
    static void get_ptr(const T &t, call_pptr p) {
        *p = reinterpret_cast<data_ptr>(t);
    }
};

template <typename T>
struct callable_pointer<T*, std::enable_if_t<trait::is_func_v<T>>> {
    static void get_ptr(const T *t, call_pptr p) {
        *p = reinterpret_cast<data_ptr>(t);
    }
};

template <typename T>
struct callable_pointer<T, std::enable_if_t<trait::is_pmf_v<T>>> {
    static void get_ptr(const T &t, call_pptr p) {
        *p = *const_cast<call_pptr>(reinterpret_cast<const void*const*>(&t));
    }
};

// for function objects, the assumption is that we are looking for the call operator
template <typename T>
struct callable_pointer<T, std::enable_if_t<trait::has_call_operator_v<T>>> {
    static void get_ptr(const T &/*t*/, call_pptr p) {
        using U = decltype(&T::operator());
        callable_pointer<U>::get_ptr(&T::operator(), p);
    }
};

template <typename T>
void get_callable_ptr(const T &t, call_pptr p) {
    callable_pointer<T>::get_ptr(t, p);
}

template <typename T>
data_ptr get_object_ptr(const T &t);

template <typename T, typename = void>
struct object_pointer {
    static data_ptr get_ptr(const T&) {
        return nullptr;
    }
};

template <typename T>
struct object_pointer<T*, std::enable_if_t<trait::is_pointer_v<T*>>> {
    static data_ptr get_ptr(const T *t) {
        return reinterpret_cast<data_ptr>(t);
    }
};

template <typename T>
struct object_pointer<T, std::enable_if_t<trait::is_weak_ptr_v<T>>> {
    static data_ptr get_ptr(const T &t) {
        auto p = t.lock();
        return get_object_ptr(p);
    }
};

template <typename T>
struct object_pointer<T, std::enable_if_t<!trait::is_pointer_v<T> &&
                                          !trait::is_weak_ptr_v<T> &&
                                          trait::is_weak_ptr_compatible_v<T>>>
{
    static data_ptr get_ptr(const T &t) {
        return t ? reinterpret_cast<data_ptr>(t.get()) : nullptr;
    }
};

template <typename T>
data_ptr get_object_ptr(const T &t) {
    return object_pointer<T>::get_ptr(t);
}


// noop mutex for thread-unsafe use
struct null_mutex {
    null_mutex() = default;
    ~null_mutex() = default;
    null_mutex(const null_mutex &) = delete;
    null_mutex operator=(const null_mutex &) = delete;
    null_mutex(null_mutex &&) = delete;
    null_mutex operator=(null_mutex &&) = delete;

    inline bool try_lock() noexcept { return true; }
    inline void lock() noexcept {}
    inline void unlock() noexcept {}
};

/**
 * A simple copy on write container that will be used to improve slot lists
 * access efficiency in a multithreaded context.
 */
template <typename T>
class copy_on_write {
    struct payload {
        payload() = default;

        template <typename... Args>
        explicit payload(Args && ...args)
            : value(std::forward<Args>(args)...)
        {}

        std::atomic<std::size_t> count{1};
        T value;
    };

public:
    using element_type = T;

    copy_on_write()
        : m_data(new payload)
    {}

    template <typename U>
    explicit copy_on_write(U && x, std::enable_if_t<!std::is_same<std::decay_t<U>,
                           copy_on_write>::value>* = nullptr)
        : m_data(new payload(std::forward<U>(x)))
    {}

    copy_on_write(const copy_on_write &x) noexcept
        : m_data(x.m_data)
    {
        ++m_data->count;
    }

    copy_on_write(copy_on_write && x) noexcept
        : m_data(x.m_data)
    {
        x.m_data = nullptr;
    }

    ~copy_on_write() {
        if (m_data && (--m_data->count == 0)) {
            delete m_data;
        }
    }

    copy_on_write& operator=(const copy_on_write &x) noexcept {
        if (&x != this) {
            *this = copy_on_write(x);
        }
        return *this;
    }

    copy_on_write& operator=(copy_on_write && x) noexcept  {
        auto tmp = std::move(x);
        swap(*this, tmp);
        return *this;
    }

    element_type& write() {
        if (!unique()) {
            *this = copy_on_write(read());
        }
        return m_data->value;
    }

    const element_type& read() const noexcept {
        return m_data->value;
    }

    friend inline void swap(copy_on_write &x, copy_on_write &y) noexcept {
        using std::swap;
        swap(x.m_data, y.m_data);
    }

private:
    bool unique() const noexcept {
        return m_data->count == 1;
    }

private:
    payload *m_data;
};

/**
 * Specializations for thread-safe code path
 */
template <typename T>
const T& cow_read(T &v) {
    return v;
}

template <typename T>
const T& cow_read(copy_on_write<T> &v) {
    return v.read();
}

template <typename T>
T& cow_write(T &v) {
    return v;
}

template <typename T>
T& cow_write(copy_on_write<T> &v) {
    return v.write();
}

/**
 * std::make_shared instantiates a lot a templates, and makes both compilation time
 * and executable size far bigger than they need to be. We offer a make_shared
 * equivalent that will avoid most instantiations with the following tradeoffs:
 * - Not exception safe,
 * - Allocates a separate control block, and will thus make the code slower.
 */
#ifdef SIGSLOT_REDUCE_COMPILE_TIME
template <typename B, typename D, typename ...Arg>
inline std::shared_ptr<B> make_shared(Arg && ... arg) {
    return std::shared_ptr<B>(static_cast<B*>(new D(std::forward<Arg>(arg)...)));
}
#else
template <typename B, typename D, typename ...Arg>
inline std::shared_ptr<B> make_shared(Arg && ... arg) {
    return std::static_pointer_cast<B>(std::make_shared<D>(std::forward<Arg>(arg)...));
}
#endif

/* slot_state holds slot type independent state, to be used to interact with
 * slots indirectly through connection and scoped_connection objects.
 */
class slot_state {
public:
    constexpr slot_state() noexcept
        : m_group(0)
        , m_index(0)
        , m_connected(true)
        , m_blocked(false)
    {}

    virtual ~slot_state() = default;

    virtual bool connected() const noexcept { return m_connected; }

    bool disconnect() noexcept {
        bool ret = m_connected.exchange(false);
        if (ret) {
            do_disconnect();
        }
        return ret;
    }

    bool blocked() const noexcept { return m_blocked.load(); }
    void block()   noexcept { m_blocked.store(true); }
    void unblock() noexcept { m_blocked.store(false); }

protected:
    virtual void do_disconnect() {}

    std::uint32_t& group() {
        return m_group;
    }

    std::uint32_t& index() {
        return m_index;
    }

private:
    template <typename, typename...>
    friend class ::sigslot::signal_base;

    std::uint32_t m_group;  // slot group this slot belongs to
    std::uint32_t m_index;  // index into the array of slot pointers inside the signal
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
    explicit connection_blocker(std::weak_ptr<detail::slot_state> s) noexcept
        : m_state{std::move(s)}
    {
        if (auto d = m_state.lock()) {
            d->block();
        }
    }

    void release() noexcept {
        if (auto d = m_state.lock()) {
            d->unblock();
        }
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
        if (auto d = m_state.lock()) {
            d->block();
        }
    }

    void unblock() noexcept {
        if (auto d = m_state.lock()) {
            d->unblock();
        }
    }

    connection_blocker blocker() const noexcept {
        return connection_blocker{m_state};
    }

protected:
    template <typename, typename...> friend class signal_base;
    explicit connection(std::weak_ptr<detail::slot_state> s) noexcept
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
    ~scoped_connection() override {
        disconnect();
    }

    /*implicit*/ scoped_connection(const connection &c) noexcept : connection(c) {}
    /*implicit*/ scoped_connection(connection &&c) noexcept : connection(std::move(c)) {}

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
    explicit scoped_connection(std::weak_ptr<detail::slot_state> s) noexcept
        : connection{std::move(s)}
    {}
};


namespace detail {

// interface for cleanable objects, used to cleanup disconnected slots
struct cleanable {
    virtual ~cleanable() = default;
    virtual void clean(slot_state *) = 0;
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
    using base_types = trait::typelist<Args...>;

    explicit slot_base(cleanable &c) : cleaner(c) {}
    ~slot_base() override = default;

    // method effectively responsible for calling the "slot" function with
    // supplied arguments whenever emission happens.
    virtual void call_slot(Args...) = 0;

    template <typename... U>
    void operator()(U && ...u) {
        if (slot_state::connected() && !slot_state::blocked()) {
            call_slot(std::forward<U>(u)...);
        }
    }

    // retieve a pointer to the object embedded in the slot
    virtual data_ptr get_object() const noexcept {
        return nullptr;
    }

    // retieve a pointer to the callable embedded in the slot
    virtual void get_callable(call_pptr p) const noexcept {
        *p = nullptr;
    }

protected:

    void do_disconnect() final {
        cleaner.clean(this);
    }

private:
    cleanable &cleaner;
};

/*
 * A slot object holds state information, and a callable to to be called
 * whenever the function call operator of its slot_base base class is called.
 */
template <typename Func, typename... Args>
class slot : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot(cleanable &c, F && f)
        : slot_base<Args...>(c)
        , func{std::forward<F>(f)} {}

protected:
    void call_slot(Args ...args) override {
        func(args...);
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(func, p);
    }

private:
    std::decay_t<Func> func;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Func, typename... Args>
class slot_extended : public slot_base<Args...> {
public:
    template <typename F>
    constexpr slot_extended(cleanable &c, F && f)
        : slot_base<Args...>(c)
        , func{std::forward<F>(f)} {}

    void call_slot(Args ...args) override {
        func(conn, args...);
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(func, p);
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
class slot_pmf : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_pmf(cleanable &c, F && f, P && p)
        : slot_base<Args...>(c)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)} {}

    void call_slot(Args ...args) override {
        ((*ptr).*pmf)(args...);
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(pmf, p);
    }

    data_ptr get_object() const noexcept override {
        return get_object_ptr(ptr);
    }

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<Ptr> ptr;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Pmf, typename Ptr, typename... Args>
class slot_pmf_extended : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_pmf_extended(cleanable &c, F && f, P && p)
        : slot_base<Args...>(c)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)} {}

    void call_slot(Args ...args) override {
        ((*ptr).*pmf)(conn, args...);
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(pmf, p);
    }

    data_ptr get_object() const noexcept override {
        return get_object_ptr(ptr);
    }

    connection conn;

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<Ptr> ptr;
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
    constexpr slot_tracked(cleanable &c, F && f, P && p)
        : slot_base<Args...>(c)
        , func{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {}

    bool connected() const noexcept override {
        return !ptr.expired() && slot_state::connected();
    }

    void call_slot(Args ...args) override {
        auto sp = ptr.lock();
        if (!sp) {
            slot_state::disconnect();
            return;
        }
        if (slot_state::connected()) {
            func(args...);
        }
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(func, p);
    }

    data_ptr get_object() const noexcept override {
        return get_object_ptr(ptr);
    }

private:
    std::decay_t<Func> func;
    std::decay_t<WeakPtr> ptr;
};

/*
 * An implementation of a slot as a pointer over member function, that tracks
 * the life of a supplied object through a weak pointer in order to automatically
 * disconnect the slot on said object destruction.
 */
template <typename Pmf, typename WeakPtr, typename... Args>
class slot_pmf_tracked : public slot_base<Args...> {
public:
    template <typename F, typename P>
    constexpr slot_pmf_tracked(cleanable &c, F && f, P && p)
        : slot_base<Args...>(c)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {}

    bool connected() const noexcept override {
        return !ptr.expired() && slot_state::connected();
    }

    void call_slot(Args ...args) override {
        auto sp = ptr.lock();
        if (!sp) {
            slot_state::disconnect();
            return;
        }
        if (slot_state::connected()) {
            ((*sp).*pmf)(args...);
        }
    }

    void get_callable(call_pptr p) const noexcept override {
        get_callable_ptr(pmf, p);
    }

    data_ptr get_object() const noexcept override {
        return get_object_ptr(ptr);
    }

private:
    std::decay_t<Pmf> pmf;
    std::decay_t<WeakPtr> ptr;
};

} // namespace detail


/**
 * signal_base is an implementation of the observer pattern, through the use
 * of an emitting object and slots that are connected to the signal and called
 * with supplied arguments when a signal is emitted.
 *
 * signal_base is the general implementation, whose locking policy must be
 * set in order to decide thread safety guarantees. signal and signal_st
 * are partial specializations for multi-threaded and single-threaded use.
 *
 * It does not allow slots to return a value.
 *
 * Slot execution order can be constrained by assigning group ids to the slots.
 * The execution order of slots in a same group in unspecified and should not be
 * relied upon, however groups are executed in ascending group ids order. When
 * the group id of a slot is not set, it is assigned to the group 0.
 * It is recommended to use small id numbers, as the groups are stored in a
 * vector a the given id index.
 *
 * @tparam Lockable a lock type to decide the lock policy
 * @tparam T... the argument types of the emitting and slots functions.
 */
template <typename Lockable, typename... T>
class signal_base : public detail::cleanable {
    template <typename L>
    using is_thread_safe = std::integral_constant<bool, !std::is_same<L, detail::null_mutex>::value>;

    template <typename U, typename L>
    using cow_type = std::conditional_t<is_thread_safe<L>::value,
                                        detail::copy_on_write<U>, U>;

    template <typename U, typename L>
    using cow_copy_type = std::conditional_t<is_thread_safe<L>::value,
                                             detail::copy_on_write<U>, const U&>;

    using lock_type = std::unique_lock<Lockable>;
    using slot_base = detail::slot_base<T...>;
    using slot_ptr = detail::slot_ptr<T...>;
    using group_type = std::vector<slot_ptr>;
    using list_type = std::vector<group_type>;

    cow_copy_type<list_type, Lockable> slots_copy() {
        lock_type lock(m_mutex);
        return m_slots;
    }

public:
    using arg_list = trait::typelist<T...>;
    using ext_arg_list = trait::typelist<connection&, T...>;

    signal_base() noexcept : m_block(false) {}
    ~signal_base() override {
        disconnect_all();
    }

    signal_base(const signal_base&) = delete;
    signal_base & operator=(const signal_base&) = delete;

    signal_base(signal_base && o) /* not noexcept */
        : m_block{o.m_block.load()}
    {
        lock_type lock(o.m_mutex);
        std::swap(m_slots, o.m_slots);
    }

    signal_base & operator=(signal_base && o) /* not noexcept */ {
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
    template <typename... U>
    void operator()(U && ...a) {
        if (m_block) {
            return;
        }

        // copy slots to execute them out of the lock
        auto copy = slots_copy();

        for (const auto &group : detail::cow_read(copy)) {
            for (const auto &s : group) {
                s->operator()(a...);
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
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    std::enable_if_t<trait::is_callable_v<arg_list, Callable>, connection>
    connect(Callable && c, std::uint32_t group = 0) {
        using slot_t = detail::slot<Callable, T...>;
        auto s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Callable>(c));
        connection conn(s);
        add_slot(std::move(s), group);
        return conn;
    }

    /**
     * Connect a callable with an additional connection argument
     *
     * The callable's first argument must be of type connection. This overload
     * the callable to manage it's own connection through this argument.
     *
     * @param c a callable
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    std::enable_if_t<trait::is_callable_v<ext_arg_list, Callable>, connection>
    connect_extended(Callable && c, std::uint32_t group = 0) {
        using slot_t = detail::slot_extended<Callable, T...>;
        auto s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Callable>(c));
        connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s), group);
        return conn;
    }

    /**
     * Overload of connect for pointers over member functions
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    std::enable_if_t<trait::is_callable_v<arg_list, Pmf, Ptr> &&
                     !trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect(Pmf && pmf, Ptr && ptr, std::uint32_t group = 0) {
        using slot_t = detail::slot_pmf<Pmf, Ptr, T...>;
        slot_ptr s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Pmf>(pmf), std::forward<Ptr>(ptr));
        connection conn(s);
        add_slot(std::move(s), group);
        return conn;
    }

    /**
     * Overload  of connect for pointer over member functions and
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    std::enable_if_t<trait::is_callable_v<ext_arg_list, Pmf, Ptr> &&
                     !trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect_extended(Pmf && pmf, Ptr && ptr, std::uint32_t group = 0) {
        using slot_t = detail::slot_pmf_extended<Pmf, Ptr, T...>;
        auto s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Pmf>(pmf), std::forward<Ptr>(ptr));
        connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s), group);
        return conn;
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
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    std::enable_if_t<!trait::is_callable_v<arg_list, Pmf> &&
                     trait::is_weak_ptr_compatible_v<Ptr>, connection>
    connect(Pmf && pmf, Ptr && ptr, std::uint32_t group = 0) {
        using trait::to_weak;
        auto w = to_weak(std::forward<Ptr>(ptr));
        using slot_t = detail::slot_pmf_tracked<Pmf, decltype(w), T...>;
        auto s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Pmf>(pmf), w);
        connection conn(s);
        add_slot(std::move(s), group);
        return conn;
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
     * @param group an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable, typename Trackable>
    std::enable_if_t<trait::is_callable_v<arg_list, Callable> &&
                     trait::is_weak_ptr_compatible_v<Trackable>, connection>
    connect(Callable && c, Trackable && ptr, std::uint32_t group = 0) {
        using trait::to_weak;
        auto w = to_weak(std::forward<Trackable>(ptr));
        using slot_t = detail::slot_tracked<Callable, decltype(w), T...>;
        auto s = detail::make_shared<slot_base, slot_t>(*this, std::forward<Callable>(c), w);
        connection conn(s);
        add_slot(std::move(s), group);
        return conn;
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
     * Disconnect slots bound to a callable
     *
     * Effect: Disconnects all the slots bound to the callable in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * The callable may be a function, a pointer to member functione a function
     * object or a (reference to a) lambda.
     *
     * @param c a callable
     * @return the number of disconnected slots
     */
    template <typename Callable>
    std::enable_if_t<trait::is_callable_v<arg_list, Callable> ||
                     trait::is_callable_v<ext_arg_list, Callable> ||
                     trait::is_pmf_v<Callable>, size_t>
    disconnect(Callable c) {
        detail ::data_ptr cp = nullptr;
        detail::get_callable_ptr(c, &cp);

        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        size_t count = 0;

        for (auto &group : groups) {
            size_t i = 0;
            while (i < group.size()) {
                detail::data_ptr cp1 = nullptr;
                group[i]->get_callable(&cp1);
                if (cp && cp1 && cp == cp1) {
                    remove_slot(group, i);
                    ++count;
                } else {
                    ++i;
                }
            }
        }

        return count;
    }

    /**
     * Disconnect slots bound to this object
     *
     * Effect: Disconnects all the slots bound to the object or tracked object
     *         in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * The object may be a pointer or trackable object.
     *
     * @param obj an object
     * @return the number of disconnected slots
     */
    template <typename Obj>
    std::enable_if_t<!trait::is_callable_v<arg_list, Obj> &&
                     !trait::is_callable_v<ext_arg_list, Obj> &&
                     !trait::is_pmf_v<Obj>, size_t>
    disconnect(const Obj &obj) {
        auto op = detail::get_object_ptr(obj);

        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        size_t count = 0;

        for (auto &group : groups) {
            size_t i = 0;
            while (i < group.size()) {
                if (group[i]->get_object() == op) {
                    remove_slot(group, i);
                    ++count;
                } else {
                    ++i;
                }
            }
        }

        return count;
    }

    /**
     * Disconnect slots bound both to a callable and object
     *
     * Effect: Disconnects all the slots bound to the callable and object in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * For naked pointers, the Callable is expected to be a pointer over member
     * function. If obj is trackable, any kind of Callable can be used.
     *
     * @param c a callable
     * @param obj an object
     * @return the number of disconnected slots
     */
    template <typename Callable, typename Obj>
    size_t disconnect(Callable c, const Obj &obj) {
        detail ::data_ptr cp = nullptr;
        detail::get_callable_ptr(c, &cp);
        auto op = detail::get_object_ptr(obj);

        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        size_t count = 0;

        for (auto &group : groups) {
            size_t i = 0;
            while (i < group.size()) {
                detail::data_ptr scp = nullptr;
                group[i]->get_callable(&scp);
                auto sop = group[i]->get_object();
                if (sop == op && scp && cp && scp == cp) {
                    remove_slot(group, i);
                    ++count;
                } else {
                    ++i;
                }
            }
        }

        return count;
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

    /**
     * Get number of connected slots
     * Safety: thread safe
     */
    size_t slot_count() noexcept {
        auto copy = slots_copy();
        size_t count = 0;
        for (auto &group : detail::cow_read(copy)) {
            count += group.size();
        }
        return count;
    }

protected:
    /**
     * remove disconnected slots
     */
    void clean(detail::slot_state *state) override {
        lock_type lock(m_mutex);
        const auto gid = state->group();
        const auto idx = state->index();

        auto &groups = detail::cow_write(m_slots);
        assert(!groups.empty());
        assert(gid < groups.size());

        auto &group = groups[gid];
        assert(idx < group.size());
        assert(group[idx].get() == state);

        remove_slot(group, idx);
    }

private:
    void add_slot(slot_ptr &&s, std::uint32_t group = 0) {
        lock_type lock(m_mutex);
        auto &ss = detail::cow_write(m_slots);
        if (ss.size() <= group) {
            ss.resize(group + 1);
        }

        s->group() = group;
        s->index() = static_cast<std::uint32_t>(ss[group].size());
        ss[group].push_back(std::move(s));
    }

    // to be called under lock
    void remove_slot(group_type &group, std::size_t idx) {
        std::swap(group[idx], group.back());
        group[idx]->index() = static_cast<std::uint32_t>(idx);
        group.pop_back();
    }

    // to be called under lock
    void clear() {
        detail::cow_write(m_slots).clear();
    }

private:
    Lockable m_mutex;
    cow_type<list_type, Lockable> m_slots;
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
 * Recursive signal emission and emission cycles are supported too.
 */
template <typename... T>
using signal = signal_base<std::mutex, T...>;

} // namespace sigslot
