#include "test-common.h"
#include <cassert>
#include <sigslot/signal.hpp>

struct copy_counter {
    copy_counter() = default;
    copy_counter(copy_counter&&) = default;
    copy_counter& operator=(copy_counter&&) = default;

    copy_counter(const copy_counter&) {
        ++numCopies;
    }

    copy_counter& operator=(const copy_counter&) {
        ++numCopies;
        return *this;
    }

    static int numCopies;
};

int copy_counter::numCopies = 0;

void test_argument_copies_for_lvalue() {
    copy_counter::numCopies = 0;

    sigslot::signal<copy_counter> sig;
    sig.connect([] (copy_counter) {});

    copy_counter c;
    sig(c);

     assert(copy_counter::numCopies == 1);
}

void test_argument_copies_for_rvalue() {
    copy_counter::numCopies = 0;

    sigslot::signal<copy_counter> sig;
    sig.connect([] (copy_counter) {});

    sig(copy_counter{});

    assert(copy_counter::numCopies == 0);
}

int main() {
    test_argument_copies_for_lvalue();
    test_argument_copies_for_rvalue();
    return 0;
}
