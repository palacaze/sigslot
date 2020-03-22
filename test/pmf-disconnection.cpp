#include "test-common.h"
#include <sigslot/signal.hpp>
#include <cassert>

#include <iostream>

// Test of complex disconnection of pointer to member function scenarii,
// to ensure it copes well with any function type.

static int sum = 0;

struct b1 {
    virtual ~b1() = default;
    static void sm() { sum++; }
    void m() { sum++; }
    virtual void vm() { sum++; }
};

struct b2 {
    virtual ~b2() = default;
    static void sm() { sum++; }
    void m() { sum++; }
    virtual void vm() { sum++; }
};

struct c {
    virtual ~c() = default;
    virtual void w() {}
};

struct d : b1 {
    static void sm() { sum++; }
    void m() { sum++; }
    void vm() override { sum++; }
};

struct e : b1, c {
    static void sm() { sum++; }
    void m() { sum++; }
    void vm() override { sum++; }
};

int main(int, char **) {
    sigslot::signal<> sig;

    auto sb1 = std::make_shared<b1>();
    auto sb2 = std::make_shared<b2>();
    auto sd = std::make_shared<d>();
    auto se = std::make_shared<e>();

    sig.connect(&b1::sm);
    sig.connect(&b1::m, sb1);
    sig.connect(&b1::vm, sb1);
    sig.connect(&b2::sm);
    sig.connect(&b2::m, sb2);
    sig.connect(&b2::vm, sb2);
    sig.connect(&d::sm);
    sig.connect(&d::m, sd);
    sig.connect(&d::vm, sd);
    sig.connect(&e::sm);
    sig.connect(&e::m, se);
    sig.connect(&e::vm, se);

    sig();
    assert(sum == 12);

#ifdef SIGSLOT_RTTI_ENABLED
    size_t n = 0;
    n = sig.disconnect(&b1::sm);
    assert(n == 1);
    n = sig.disconnect(&b1::m);
    assert(n == 1);
    n = sig.disconnect(&b1::vm);
    assert(n == 1);
    n = sig.disconnect(&b2::sm);
    assert(n == 1);
    n = sig.disconnect(&b2::m);
    assert(n == 1);
    n = sig.disconnect(&b2::vm);
    assert(n == 1);
    n = sig.disconnect(&d::sm);
    assert(n == 1);
    n = sig.disconnect(&d::m);
    assert(n == 1);
    n = sig.disconnect(&d::vm);
    assert(n == 1);
    n = sig.disconnect(&e::sm);
    assert(n == 1);
    n = sig.disconnect(&e::m);
    assert(n == 1);
    n = sig.disconnect(&e::vm);
    assert(n == 1);
#endif

    return 0;
}
