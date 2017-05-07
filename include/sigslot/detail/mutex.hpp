#pragma once

namespace pal {
namespace detail {

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
} // namespace pal
