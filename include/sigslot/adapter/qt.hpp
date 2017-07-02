#pragma once
#include <type_traits>
#include <QtGlobal>
#include <QSharedPointer>
#include <QPointer>
#include <QObject>

/**
 * Definition of a few adapters that allow object life tracking for QObject,
 * QWeakPointer and QSharedPointer objects, when connected to signals.
 */

namespace sigslot {
namespace detail {

// a weak pointer adater to allow QObject life tracking
template <typename T>
struct qpointer_adater {
    qpointer_adater(T *o) noexcept
        : m_ptr{o}
    {}

    void reset() noexcept {
        m_ptr.clear();
    }

    bool expired() const noexcept {
        return m_ptr.isNull();
    }

    // Warning: very unsafe because QPointer does not provide weak pointer semantics
    // In a multithreaded context, m_ptr may very well be destroyed in the lapse of
    // time between expired() and data() (and also while data() is being used).
    T* lock() const noexcept {
        return expired() ? nullptr : m_ptr.data();
    }

private:
    QPointer<T> m_ptr;
};

// a wrapper that exposes the right concepts for QWeakPointer
template <typename T>
struct qweakpointer_adater {
    qweakpointer_adater(QWeakPointer<T> o) noexcept
        : m_ptr{std::move(o)}
    {}

    void reset() noexcept {
        m_ptr.clear();
    }

    bool expired() const noexcept {
        return m_ptr.isNull();
    }

    QSharedPointer<T> lock() const noexcept {
        return m_ptr.lock();
    }

private:
    QWeakPointer<T> m_ptr;
};

} // namespace detail
} // namespace sigslot


QT_BEGIN_NAMESPACE

template <typename T>
std::enable_if_t<std::is_base_of<QObject, T>::value, sigslot::detail::qpointer_adater<T>>
to_weak(T *p) {
    return {p};
}

template <typename T>
sigslot::detail::qweakpointer_adater<T> to_weak(QWeakPointer<T> p) {
    return {p};
}

template <typename T>
sigslot::detail::qweakpointer_adater<T> to_weak(QSharedPointer<T> p) {
    return {p};
}

QT_END_NAMESPACE

