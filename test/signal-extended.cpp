#include <sigslot/signal.hpp>
#include <cassert>

using namespace pal;

int sum = 0;

void f(connection &c, int i) { sum += i; c.disconnect(); }

struct s {
    static void sf(connection &c, int i) { sum += i; c.disconnect(); }
    void f(connection &c, int i) { sum += i; c.disconnect(); }
};

struct o {
    void operator()(connection &c, int i) { sum += i; c.disconnect(); }
};

void test_free_connection() {
    sum = 0;
    signal<int> sig;
    sig.connect(f);

    sig(1);
    assert(sum == 1);
    sig(1);
    assert(sum == 1);
}

void test_static_connection() {
    sum = 0;
    signal<int> sig;
    sig.connect(&s::sf);

    sig(1);
    assert(sum == 1);
    sig(1);
    assert(sum == 1);
}

void test_pmf_connection() {
    sum = 0;
    signal<int> sig;
    s p;
    sig.connect(&s::f, &p);

    sig(1);
    assert(sum == 1);
    sig(1);
    assert(sum == 1);
}

void test_function_object_connection() {
    sum = 0;
    signal<int> sig;
    sig.connect(o{});

    sig(1);
    assert(sum == 1);
    sig(1);
    assert(sum == 1);
}

void test_lambda_connection() {
    sum = 0;
    signal<int> sig;

    sig.connect([&](connection &c, int i) { sum += i; c.disconnect(); });
    sig(1);
    assert(sum == 1);

    sig.connect([&](connection &c, int i) mutable { sum += 2*i; c.disconnect(); });
    sig(1);
    assert(sum == 3);
    sig(1);
    assert(sum == 3);
}

int main() {
    test_free_connection();
    test_static_connection();
    test_pmf_connection();
    test_function_object_connection();
    test_lambda_connection();
}

