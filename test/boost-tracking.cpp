#include "test-common.h"
#include <boost/make_shared.hpp>
#include <sigslot/adapter/boost.hpp>
#include <sigslot/signal.hpp>
#include <cassert>

static int sum = 0;

void f1(int i) { sum += i; }
struct o1 { void operator()(int i) { sum += 2*i; } };

struct s {
    void f1(int i) { sum += i; }
    void f2(int i) const { sum += 2*i; }
};

struct dummy {};


void test_track_shared() {
    sum = 0;
    sigslot::signal<int> sig;

    auto s1 = boost::make_shared<s>();
    sig.connect(&s::f1, s1);

    auto s2 = boost::make_shared<s>();
    boost::weak_ptr<s> w2 = s2;
    sig.connect(&s::f2, w2);

    sig(1);
    assert(sum == 3);

    s1.reset();
    sig(1);
    assert(sum == 5);

    s2.reset();
    sig(1);
    assert(sum == 5);
}

void test_track_other() {
    sum = 0;
    sigslot::signal<int> sig;

    auto d1 = boost::make_shared<dummy>();
    sig.connect(f1, d1);

    auto d2 = boost::make_shared<dummy>();
    boost::weak_ptr<dummy> w2 = d2;
    sig.connect(o1(), w2);

    sig(1);
    assert(sum == 3);

    d1.reset();
    sig(1);
    assert(sum == 5);

    d2.reset();
    sig(1);
    assert(sum == 5);
}

int main() {
    test_track_shared();
    test_track_other();
    return 0;
}
