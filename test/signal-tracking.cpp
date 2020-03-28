#include "test-common.h"
#include <sigslot/signal.hpp>
#include <string>
#include <sstream>
#include <cmath>
#include <cassert>

static int sum = 0;

void f1(int i) { sum += i; }
struct o1 { void operator()(int i) { sum += 2*i; } };

struct s {
    void f1(int i) { sum += i; }
    void f2(int i) const { sum += 2*i; }
};

struct oo {
    void operator()(int i) { sum += i; }
    void operator()(double i) { sum += static_cast<int>(std::round(4*i)); }
};

struct dummy {};

static_assert(sigslot::trait::is_callable_v<sigslot::trait::typelist<int>, decltype(&s::f1), std::shared_ptr<s>>, "");

void test_track_shared() {
    sum = 0;
    sigslot::signal<int> sig;

    auto s1 = std::make_shared<s>();
    auto conn1 = sig.connect(&s::f1, s1);

    auto s2 = std::make_shared<s>();
    std::weak_ptr<s> w2 = s2;
    auto conn2 = sig.connect(&s::f2, w2);

    sig(1);
    assert(sum == 3);

    s1.reset();
    sig(1);
    assert(sum == 5);
    assert(!conn1.valid());

    s2.reset();
    sig(1);
    assert(sum == 5);
    assert(!conn2.valid());
}

// bug #2 remove last slot first
void test_track_shared_reversed() {
    sum = 0;
    sigslot::signal<int> sig;

    auto s1 = std::make_shared<s>();
    auto conn1 = sig.connect(&s::f1, s1);

    auto s2 = std::make_shared<s>();
    std::weak_ptr<s> w2 = s2;
    auto conn2 = sig.connect(&s::f2, w2);

    sig(1);
    assert(sum == 3);

    s2.reset();
    sig(1);
    assert(sum == 4);
    assert(!conn2.valid());

    s1.reset();
    sig(1);
    assert(sum == 4);
    assert(!conn1.valid());
}

void test_track_other() {
    sum = 0;
    sigslot::signal<int> sig;

    auto d1 = std::make_shared<dummy>();
    auto conn1 = sig.connect(f1, d1);

    auto d2 = std::make_shared<dummy>();
    std::weak_ptr<dummy> w2 = d2;
    auto conn2 = sig.connect(o1(), w2);

    sig(1);
    assert(sum == 3);

    d1.reset();
    sig(1);
    assert(sum == 5);
    assert(!conn1.valid());

    d2.reset();
    sig(1);
    assert(sum == 5);
    assert(!conn2.valid());
}

void test_track_overloaded_function_object() {
    sum = 0;
    sigslot::signal<int> sig;
    sigslot::signal<double> sig1;

    auto d1 = std::make_shared<dummy>();
    auto conn1 = sig.connect(oo{}, d1);
    sig(1);
    assert(sum == 1);

    d1.reset();
    sig(1);
    assert(sum == 1);
    assert(!conn1.valid());

    auto d2 = std::make_shared<dummy>();
    std::weak_ptr<dummy> w2 = d2;
    auto conn2 = sig1.connect(oo{}, w2);
    sig1(1);
    assert(sum == 5);

    d2.reset();
    sig1(1);
    assert(sum == 5);
    assert(!conn2.valid());
}

void test_track_generic_lambda() {
    std::stringstream s;

    auto f = [&] (auto a, auto ...args) {
        using result_t = int[];
        s << a;
        result_t r{ 1, ((void)(s << args), 1)..., };
        (void)r;
    };

    sigslot::signal<int> sig1;
    sigslot::signal<std::string> sig2;
    sigslot::signal<double> sig3;

    auto d1 = std::make_shared<dummy>();
    sig1.connect(f, d1);
    sig2.connect(f, d1);
    sig3.connect(f, d1);

    sig1(1);
    sig2("foo");
    sig3(4.1);
    assert(s.str() == "1foo4.1");

    d1.reset();
    sig1(2);
    sig2("bar");
    sig3(3.0);
    assert(s.str() == "1foo4.1");
}

int main() {
    test_track_shared();
    test_track_shared_reversed();
    test_track_other();
    test_track_overloaded_function_object();
    test_track_generic_lambda();
    return 0;
}
