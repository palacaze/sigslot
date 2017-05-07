#include <sigslot/signal.hpp>
#include <cassert>

using namespace pal;

int sum = 0;

void f1(int i) { sum += i; }
void f2(int i) noexcept { sum += 2*i; }

struct s {
    static void s1(int i) { sum += i; }
    static void s2(int i) noexcept { sum += 2*i; }

    void f1(int i) { sum += i; }
    void f2(int i) const { sum += i; }
    void f3(int i) volatile { sum += i; }
    void f4(int i) const volatile { sum += i; }
    void f5(int i) noexcept { sum += i; }
    void f6(int i) const noexcept { sum += i; }
    void f7(int i) volatile noexcept { sum += i; }
    void f8(int i) const volatile noexcept { sum += i; }
};

struct o1 { void operator()(int i) { sum += i; } };
struct o2 { void operator()(int i) const { sum += i; } };
struct o3 { void operator()(int i) volatile { sum += i; } };
struct o4 { void operator()(int i) const volatile { sum += i; } };
struct o5 { void operator()(int i) noexcept { sum += i; } };
struct o6 { void operator()(int i) const noexcept { sum += i; } };
struct o7 { void operator()(int i) volatile noexcept { sum += i; } };
struct o8 { void operator()(int i) const volatile noexcept { sum += i; } };

void test_free_connection() {
    sum = 0;
    signal<int> sig;

    auto c1 = sig.connect(f1);
    sig(1);
    assert(sum == 1);

    sig.connect(f2);
    sig(1);
    assert(sum == 4);
}

void test_static_connection() {
    sum = 0;
    signal<int> sig;

    sig.connect(&s::s1);
    sig(1);
    assert(sum == 1);

    sig.connect(&s::s2);
    sig(1);
    assert(sum == 4);
}

void test_pmf_connection() {
    sum = 0;
    signal<int> sig;
    s p;

    sig.connect(&s::f1, &p);
    sig.connect(&s::f2, &p);
    sig.connect(&s::f3, &p);
    sig.connect(&s::f4, &p);
    sig.connect(&s::f5, &p);
    sig.connect(&s::f6, &p);
    sig.connect(&s::f7, &p);
    sig.connect(&s::f8, &p);

    sig(1);
    assert(sum == 8);
}

void test_const_pmf_connection() {
    sum = 0;
    signal<int> sig;
    const s p;

    sig.connect(&s::f2, &p);
    sig.connect(&s::f4, &p);
    sig.connect(&s::f6, &p);
    sig.connect(&s::f8, &p);

    sig(1);
    assert(sum == 4);
}

void test_function_object_connection() {
    sum = 0;
    signal<int> sig;

    sig.connect(o1{});
    sig.connect(o2{});
    sig.connect(o3{});
    sig.connect(o4{});
    sig.connect(o5{});
    sig.connect(o6{});
    sig.connect(o7{});
    sig.connect(o8{});

    sig(1);
    assert(sum == 8);
}

void test_lambda_connection() {
    sum = 0;
    signal<int> sig;

    sig.connect([&](int i) { sum += i; });
    sig(1);
    assert(sum == 1);

    sig.connect([&](int i) mutable { sum += 2*i; });
    sig(1);
    assert(sum == 4);
}

void test_mutation() {
    int res = 0;
    signal<int&> sig;

    sig.connect([](int &r) { r += 1; });
    sig(res);
    assert(res == 1);

    sig.connect([](int &r) mutable { r += 2; });
    sig(res);
    assert(res == 4);
}

void test_disconnection() {
    // test removing only connected
    {
        sum = 0;
        signal<int> sig;

        auto sc = sig.connect(f1);
        sig(1);
        assert(sum == 1);

        sc.disconnect();
        sig(1);
        assert(sum == 1);
    }

    // test removing first connected
    {
        sum = 0;
        signal<int> sig;

        auto sc = sig.connect(f1);
        sig(1);
        assert(sum == 1);

        sig.connect(f2);
        sig(1);
        assert(sum == 4);

        sc.disconnect();
        sig(1);
        assert(sum == 6);
    }

    // test removing last connected
    {
        sum = 0;
        signal<int> sig;

        sig.connect(f1);
        sig(1);
        assert(sum == 1);

        auto sc = sig.connect(f2);
        sig(1);
        assert(sum == 4);

        sc.disconnect();
        sig(1);
        assert(sum == 5);
    }
}

void test_scoped_connection() {
    sum = 0;
    signal<int> sig;

    {
        auto sc1 = sig.connect_scoped(f1);
        sig(1);
        assert(sum == 1);

        auto sc2 = sig.connect_scoped(f2);
        sig(1);
        assert(sum == 4);
    }

    sig(1);
    assert(sum == 4);
}

void test_connection_blocking() {
    sum = 0;
    signal<int> sig;

    auto c1 = sig.connect(f1);
    sig.connect(f2);
    sig(1);
    assert(sum == 3);

    c1.block();
    sig(1);
    assert(sum == 5);

    c1.unblock();
    sig(1);
    assert(sum == 8);
}

void test_connection_blocker() {
    sum = 0;
    signal<int> sig;

    auto c1 = sig.connect(f1);
    sig.connect(f2);
    sig(1);
    assert(sum == 3);

    {
        auto cb = c1.blocker();
        sig(1);
        assert(sum == 5);
    }

    sig(1);
    assert(sum == 8);
}

void test_signal_blocking() {
    sum = 0;
    signal<int> sig;

    sig.connect(f1);
    sig.connect(f2);
    sig(1);
    assert(sum == 3);

    sig.block();
    sig(1);
    assert(sum == 3);

    sig.unblock();
    sig(1);
    assert(sum == 6);
}

void test_all_disconnection() {
    sum = 0;
    signal<int> sig;

    sig.connect(f1);
    sig.connect(f2);
    sig(1);
    assert(sum == 3);

    sig.disconnect_all();
    sig(1);
    assert(sum == 3);
}

void test_connection_copying_moving() {
    sum = 0;
    signal<int> sig;

    auto sc1 = sig.connect(f1);
    auto sc2 = sig.connect(f2);

    auto sc3 = sc1;
    auto sc4{sc2};

    auto sc5 = std::move(sc3);
    auto sc6{std::move(sc4)};

    sig(1);
    assert(sum == 3);

    sc5.block();
    sig(1);
    assert(sum == 5);

    sc1.unblock();
    sig(1);
    assert(sum == 8);

    sc6.disconnect();
    sig(1);
    assert(sum == 9);
}

void test_scoped_connection_moving() {
    sum = 0;
    signal<int> sig;

    {
        auto sc1 = sig.connect_scoped(f1);
        sig(1);
        assert(sum == 1);

        auto sc2 = sig.connect_scoped(f2);
        sig(1);
        assert(sum == 4);

        auto sc3 = std::move(sc1);;
        sig(1);
        assert(sum == 7);

        auto sc4{std::move(sc2)};
        sig(1);
        assert(sum == 10);
    }

    sig(1);
    assert(sum == 10);
}

void test_signal_moving() {
    sum = 0;
    signal<int> sig;

    sig.connect(f1);
    sig.connect(f2);

    sig(1);
    assert(sum == 3);

    auto sig2 = std::move(sig);
    sig2(1);
    assert(sum == 6);

    auto sig3 = std::move(sig2);
    sig3(1);
    assert(sum == 9);
}

int main() {
    test_free_connection();
    test_static_connection();
    test_pmf_connection();
    test_const_pmf_connection();
    test_function_object_connection();
    test_lambda_connection();
    test_disconnection();
    test_scoped_connection();
    test_connection_blocker();
    test_connection_blocking();
    test_signal_blocking();
    test_all_disconnection();
    test_connection_copying_moving();
    test_scoped_connection_moving();
    test_signal_moving();
    return 0;
}

