#include "test-common.h"
#include <cassert>
#include <sigslot/signal.hpp>

static constexpr sigslot::group_id grps = 30;
static constexpr int64_t slts = 3;
static constexpr int64_t emissions = 10000;
static constexpr int64_t runs = 1000;

static void fun(int64_t &i) { i++; }

static void test_groups(int64_t &i) {
    sigslot::signal<int64_t&> sig;

    for (int64_t s = 0; s < slts; ++s) {
        for (sigslot::group_id g = 0; g < grps; ++g) {
            sig.connect(fun, grps-g);
        }
    }

    for (int64_t e = 0; e < emissions; ++e) {
        sig(i);
    }
}

int main(int, char **) {
    int64_t i = 0;

    for (int64_t r = 0; r < runs; ++r) {
        test_groups(i);
    }

    assert(i == grps * slts * emissions * runs);
    return 0;
}
