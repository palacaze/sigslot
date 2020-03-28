#include "test-common.h"
#include <sigslot/signal.hpp>
#include <string>
#include <sstream>
#include <cassert>
#include <cmath>

static int sum = 0;

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

struct oo {
    void operator()(int i) { sum += i; }
    void operator()(double i) { sum += int(std::round(4*i)); }
};

struct o1 { void operator()(int i) { sum += i; } };
struct o2 { void operator()(int i) const { sum += i; } };
struct o3 { void operator()(int i) volatile { sum += i; } };
struct o4 { void operator()(int i) const volatile { sum += i; } };
struct o5 { void operator()(int i) noexcept { sum += i; } };
struct o6 { void operator()(int i) const noexcept { sum += i; } };
struct o7 { void operator()(int i) volatile noexcept { sum += i; } };
struct o8 { void operator()(int i) const volatile noexcept { sum += i; } };

void test_slot_count() {
    sigslot::signal<int> sig;
    s p;

    sig.connect(&s::f1, &p);
    assert(sig.slot_count() == 1);
    sig.connect(&s::f2, &p);
    assert(sig.slot_count() == 2);
    sig.connect(&s::f3, &p);
    assert(sig.slot_count() == 3);
    sig.connect(&s::f4, &p);
    assert(sig.slot_count() == 4);
    sig.connect(&s::f5, &p);
    assert(sig.slot_count() == 5);
    sig.connect(&s::f6, &p);
    assert(sig.slot_count() == 6);

    {
        sigslot::scoped_connection conn = sig.connect(&s::f7, &p);
        assert(sig.slot_count() == 7);
    }
    assert(sig.slot_count() == 6);

    auto conn = sig.connect(&s::f8, &p);
    assert(sig.slot_count() == 7);
    conn.disconnect();
    assert(sig.slot_count() == 6);

    sig.disconnect_all();
    assert(sig.slot_count() == 0);
}

void test_free_connection() {
    sum = 0;
    sigslot::signal<int> sig;

    auto c1 = sig.connect(f1);
    sig(1);
    assert(sum == 1);

    sig.connect(f2);
    sig(1);
    assert(sum == 4);
}

void test_static_connection() {
    sum = 0;
    sigslot::signal<int> sig;

    sig.connect(&s::s1);
    sig(1);
    assert(sum == 1);

    sig.connect(&s::s2);
    sig(1);
    assert(sum == 4);
}

void test_pmf_connection() {
    sum = 0;
    sigslot::signal<int> sig;
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
    sigslot::signal<int> sig;
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
    sigslot::signal<int> sig;

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

void test_overloaded_function_object_connection() {
    sum = 0;
    sigslot::signal<int> sig;
    sigslot::signal<double> sig1;

    sig.connect(oo{});
    sig(1);
    assert(sum == 1);

    sig1.connect(oo{});
    sig1(1);
    assert(sum == 5);
}

void test_lambda_connection() {
    sum = 0;
    sigslot::signal<int> sig;

    sig.connect([&](int i) { sum += i; });
    sig(1);
    assert(sum == 1);

    sig.connect([&](int i) mutable { sum += 2*i; });
    sig(1);
    assert(sum == 4);
}

void test_generic_lambda_connection() {
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

    sig1.connect(f);
    sig2.connect(f);
    sig3.connect(f);
    sig1(1);
    sig2("foo");
    sig3(4.1);

    assert(s.str() == "1foo4.1");
}

void test_lvalue_emission() {
    sum = 0;
    sigslot::signal<int> sig;

    auto c1 = sig.connect(f1);
    int v = 1;
    sig(v);
    assert(sum == 1);

    sig.connect(f2);
    sig(v);
    assert(sum == 4);
}

void test_mutation() {
    int res = 0;
    sigslot::signal<int&> sig;

    sig.connect([](int &r) { r += 1; });
    sig(res);
    assert(res == 1);

    sig.connect([](int &r) mutable { r += 2; });
    sig(res);
    assert(res == 4);
}

void test_compatible_args() {
    long ll = 0;
    std::string ss;
    short ii = 0;

    auto f = [&] (long l, const std::string &s, short i) {
        ll = l; ss = s; ii = i;
    };

    sigslot::signal<int, std::string, bool> sig;
    sig.connect(f);
    sig('0', "foo", true);

    assert(ll == 48);
    assert(ss == "foo");
    assert(ii == 1);
}

void test_disconnection() {
    // test removing only connected
    {
        sum = 0;
        sigslot::signal<int> sig;

        auto sc = sig.connect(f1);
        sig(1);
        assert(sum == 1);

        sc.disconnect();
        sig(1);
        assert(sum == 1);
        assert(!sc.valid());
    }

    // test removing first connected
    {
        sum = 0;
        sigslot::signal<int> sig;

        auto sc = sig.connect(f1);
        sig(1);
        assert(sum == 1);

        sig.connect(f2);
        sig(1);
        assert(sum == 4);

        sc.disconnect();
        sig(1);
        assert(sum == 6);
        assert(!sc.valid());
    }

    // test removing last connected
    {
        sum = 0;
        sigslot::signal<int> sig;

        sig.connect(f1);
        sig(1);
        assert(sum == 1);

        auto sc = sig.connect(f2);
        sig(1);
        assert(sum == 4);

        sc.disconnect();
        sig(1);
        assert(sum == 5);
        assert(!sc.valid());
    }
}

void test_disconnection_by_callable() {
    // disconnect a function pointer
    {
        sum = 0;
        sigslot::signal<int> sig;

        sig.connect(f1);
        sig.connect(f2);
        sig.connect(f2);
        sig(1);
        assert(sum == 5);
        auto c = sig.disconnect(&f2);
        assert(c == 2);
        sig(1);
        assert(sum == 6);
    }

    // disconnect a function
    {
        sum = 0;
        sigslot::signal<int> sig;

        sig.connect(f1);
        sig.connect(f2);
        sig(1);
        assert(sum == 3);
        sig.disconnect(f1);
        sig(1);
        assert(sum == 5);
    }

#ifdef SIGSLOT_RTTI_ENABLED
    // disconnect by pmf
    {
        sum = 0;
        sigslot::signal<int> sig;
        s p;

        sig.connect(&s::f1, &p);
        sig.connect(&s::f2, &p);
        sig(1);
        assert(sum == 2);
        sig.disconnect(&s::f1);
        sig(1);
        assert(sum == 3);
    }

    // disconnect by function object
    {
        sum = 0;
        sigslot::signal<int> sig;

        sig.connect(o1{});
        sig.connect(o2{});
        sig(1);
        assert(sum == 2);
        sig.disconnect(o1{});
        sig(1);
        assert(sum == 3);
    }

    // disconnect by lambda
    {
        sum = 0;
        sigslot::signal<int> sig;
        auto l1 = [&](int i) { sum += i; };
        auto l2 = [&](int i) { sum += 2*i; };
        sig.connect(l1);
        sig.connect(l2);
        sig(1);
        assert(sum == 3);
        sig.disconnect(l1);
        sig(1);
        assert(sum == 5);
    }
#endif
}

void test_disconnection_by_object() {
    // disconnect by pointer
    {
        sum = 0;
        sigslot::signal<int> sig;
        s p1, p2;

        sig.connect(&s::f1, &p1);
        sig.connect(&s::f2, &p2);
        sig(1);
        assert(sum == 2);
        sig.disconnect(&p1);
        sig(1);
        assert(sum == 3);
    }

    // disconnect by shared pointer
    {
        sum = 0;
        sigslot::signal<int> sig;
        auto p1 = std::make_shared<s>();
        s p2;

        sig.connect(&s::f1, p1);
        sig.connect(&s::f2, &p2);
        sig(1);
        assert(sum == 2);
        sig.disconnect(p1);
        sig(1);
        assert(sum == 3);
    }
}

void test_disconnection_by_object_and_pmf() {
    // disconnect by pointer
    {
        sum = 0;
        sigslot::signal<int> sig;
        s p1, p2;

        sig.connect(&s::f1, &p1);
        sig.connect(&s::f1, &p2);
        sig.connect(&s::f2, &p1);
        sig.connect(&s::f2, &p2);
        sig(1);
        assert(sum == 4);
        sig.disconnect(&s::f1, &p2);
        sig(1);
        assert(sum == 7);
    }

    // disconnect by shared pointer
    {
        sum = 0;
        sigslot::signal<int> sig;
        auto p1 = std::make_shared<s>();
        auto p2 = std::make_shared<s>();

        sig.connect(&s::f1, p1);
        sig.connect(&s::f1, p2);
        sig.connect(&s::f2, p1);
        sig.connect(&s::f2, p2);
        sig(1);
        assert(sum == 4);
        sig.disconnect(&s::f1, p2);
        sig(1);
        assert(sum == 7);
    }

    // disconnect by tracker
    {
        sum = 0;
        sigslot::signal<int> sig;

        auto t = std::make_shared<bool>();
        sig.connect(f1);
        sig.connect(f2);
        sig.connect(f1, t);
        sig.connect(f2, t);
        sig(1);
        assert(sum == 6);
        sig.disconnect(f2, t);
        sig(1);
        assert(sum == 10);
    }
}

void test_scoped_connection() {
    sum = 0;
    sigslot::signal<int> sig;

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

    sum = 0;

    {
        sigslot::scoped_connection sc1 = sig.connect(f1);
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
    sigslot::signal<int> sig;

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
    sigslot::signal<int> sig;

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
    sigslot::signal<int> sig;

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
    sigslot::signal<int> sig;

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
    sigslot::signal<int> sig;

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
    sigslot::signal<int> sig;

    {
        auto sc1 = sig.connect_scoped(f1);
        sig(1);
        assert(sum == 1);

        auto sc2 = sig.connect_scoped(f2);
        sig(1);
        assert(sum == 4);

        auto sc3 = std::move(sc1);
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
    sigslot::signal<int> sig;

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

template <typename T>
struct object {
    object();
    object(T i) : v{i} {}

    const T & val() const { return v; }
    T & val() { return v; }
    void set_val(const T &i) {
        if (i != v) {
            v = i;
            s(i);
        }
    }

    sigslot::signal<T> & sig() { return s; }

private:
    T v;
    sigslot::signal<T> s;
};

void test_loop() {
    object<int> i1(0);
    object<int> i2(3);

    i1.sig().connect(&object<int>::set_val, &i2);
    i2.sig().connect(&object<int>::set_val, &i1);

    i1.set_val(1);

    assert(i1.val() == 1);
    assert(i2.val() == 1);
}

int main() {
    test_free_connection();
    test_static_connection();
    test_pmf_connection();
    test_const_pmf_connection();
    test_function_object_connection();
    test_overloaded_function_object_connection();
    test_lambda_connection();
    test_generic_lambda_connection();
    test_lvalue_emission();
    test_compatible_args();
    test_mutation();
    test_disconnection();
    test_disconnection_by_callable();
    test_disconnection_by_object();
    test_disconnection_by_object_and_pmf();
    test_scoped_connection();
    test_connection_blocker();
    test_connection_blocking();
    test_signal_blocking();
    test_all_disconnection();
    test_connection_copying_moving();
    test_scoped_connection_moving();
    test_signal_moving();
    test_loop();
    test_slot_count();
    return 0;
}
