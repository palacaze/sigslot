#pragma once
#include <memory>
#include <utility>
#include "detail/slot.hpp"

namespace pal {

/**
 * connectiom_blocker is a RAII object that blocks a connection until destruction
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

} // namespace pal

