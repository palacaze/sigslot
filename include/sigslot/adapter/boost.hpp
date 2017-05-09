#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

/**
 * Bring ADL conversion for boost smart pointers
 */

namespace boost {

template <typename T>
weak_ptr<T> to_weak(shared_ptr<T> p) {
    return {p};
}

template <typename T>
weak_ptr<T> to_weak(weak_ptr<T> p) {
    return p;
}

} // namespace boost

