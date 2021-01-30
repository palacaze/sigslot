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

struct owner {
    sigslot::signal_ix<owner, int> Sig;
    sigslot::signal_ix<owner, double> Sig2;
    sigslot::signal_ix<owner, std::string> Sig3;
    sigslot::signal_ix<owner, int&> Sig4;
    sigslot::signal_ix<owner, int, std::string, bool> Sig5;

    void test_slot_count() {
        s p;

        Sig.connect(&s::f1, &p);
        assert(Sig.slot_count() == 1);
        Sig.connect(&s::f2, &p);
        assert(Sig.slot_count() == 2);
        Sig.connect(&s::f3, &p);
        assert(Sig.slot_count() == 3);
        Sig.connect(&s::f4, &p);
        assert(Sig.slot_count() == 4);
        Sig.connect(&s::f5, &p);
        assert(Sig.slot_count() == 5);
        Sig.connect(&s::f6, &p);
        assert(Sig.slot_count() == 6);

        {
            sigslot::scoped_connection conn = Sig.connect(&s::f7, &p);
            assert(Sig.slot_count() == 7);
        }
        assert(Sig.slot_count() == 6);

        auto conn = Sig.connect(&s::f8, &p);
        assert(Sig.slot_count() == 7);
        conn.disconnect();
        assert(Sig.slot_count() == 6);

        Sig.disconnect_all();
        assert(Sig.slot_count() == 0);
    }

    void test_free_connection() {
        sum = 0;

        auto c1 = Sig.connect(f1);
        Sig(1);
        assert(sum == 1);

        Sig.connect(f2);
        Sig(1);
        assert(sum == 4);
        Sig.disconnect_all();
    }

    void test_static_connection() {
        sum = 0;

        Sig.connect(&s::s1);
        Sig(1);
        assert(sum == 1);

        Sig.connect(&s::s2);
        Sig(1);
        assert(sum == 4);
        Sig.disconnect_all();
    }

    void test_pmf_connection() {
        sum = 0;
        s p;

        Sig.connect(&s::f1, &p);
        Sig.connect(&s::f2, &p);
        Sig.connect(&s::f3, &p);
        Sig.connect(&s::f4, &p);
        Sig.connect(&s::f5, &p);
        Sig.connect(&s::f6, &p);
        Sig.connect(&s::f7, &p);
        Sig.connect(&s::f8, &p);

        Sig(1);
        assert(sum == 8);
        Sig.disconnect_all();
    }

    void test_const_pmf_connection() {
        sum = 0;
        const s p;

        Sig.connect(&s::f2, &p);
        Sig.connect(&s::f4, &p);
        Sig.connect(&s::f6, &p);
        Sig.connect(&s::f8, &p);

        Sig(1);
        assert(sum == 4);
        Sig.disconnect_all();
    }

    void test_function_object_connection() {
        sum = 0;

        Sig.connect(o1{});
        Sig.connect(o2{});
        Sig.connect(o3{});
        Sig.connect(o4{});
        Sig.connect(o5{});
        Sig.connect(o6{});
        Sig.connect(o7{});
        Sig.connect(o8{});

        Sig(1);
        assert(sum == 8);
        Sig.disconnect_all();
    }

    void test_overloaded_function_object_connection() {
        sum = 0;

        Sig.connect(oo{});
        Sig(1);
        assert(sum == 1);

        Sig2.connect(oo{});
        Sig2(1);
        assert(sum == 5);
        Sig.disconnect_all();
        Sig2.disconnect_all();
    }

    void test_lambda_connection() {
        sum = 0;

        Sig.connect([&](int i) { sum += i; });
        Sig(1);
        assert(sum == 1);

        Sig.connect([&](int i) mutable { sum += 2*i; });
        Sig(1);
        assert(sum == 4);
        Sig.disconnect_all();
    }

    void test_generic_lambda_connection() {
        std::stringstream s;

        auto f = [&] (auto a, auto ...args) {
            using result_t = int[];
            s << a;
            result_t r{ 1, ((void)(s << args), 1)..., };
            (void)r;
        };

        Sig.connect(f);
        Sig3.connect(f);
        Sig2.connect(f);
        Sig(1);
        Sig3("foo");
        Sig2(4.1);

        assert(s.str() == "1foo4.1");
        Sig.disconnect_all();
        Sig3.disconnect_all();
        Sig2.disconnect_all();
    }

    void test_lvalue_emission() {
        sum = 0;

        auto c1 = Sig.connect(f1);
        int v = 1;
        Sig(v);
        assert(sum == 1);

        Sig.connect(f2);
        Sig(v);
        assert(sum == 4);
        Sig.disconnect_all();
    }

    void test_mutation() {
        int res = 0;

        Sig4.connect([](int &r) { r += 1; });
        Sig4(res);
        assert(res == 1);

        Sig4.connect([](int &r) mutable { r += 2; });
        Sig4(res);
        assert(res == 4);
        Sig4.disconnect_all();
    }

    void test_compatible_args() {
        long ll = 0;
        std::string ss;
        short ii = 0;

        auto f = [&] (long l, const std::string &s, short i) {
            ll = l; ss = s; ii = i;
        };

        Sig5.connect(f);
        Sig5('0', "foo", true);

        assert(ll == 48);
        assert(ss == "foo");
        assert(ii == 1);
        Sig5.disconnect_all();
    }

    void test_disconnection() {
        // test removing only connected
        {
            sum = 0;

            auto sc = Sig.connect(f1);
            Sig(1);
            assert(sum == 1);

            sc.disconnect();
            Sig(1);
            assert(sum == 1);
            assert(!sc.valid());
        }

        // test removing first connected
        {
            sum = 0;

            auto sc = Sig.connect(f1);
            Sig(1);
            assert(sum == 1);

            Sig.connect(f2);
            Sig(1);
            assert(sum == 4);

            sc.disconnect();
            Sig(1);
            assert(sum == 6);
            assert(!sc.valid());
            Sig.disconnect_all();
        }

        // test removing last connected
        {
            sum = 0;

            Sig.connect(f1);
            Sig(1);
            assert(sum == 1);

            auto sc = Sig.connect(f2);
            Sig(1);
            assert(sum == 4);

            sc.disconnect();
            Sig(1);
            assert(sum == 5);
            assert(!sc.valid());
            Sig.disconnect_all();
        }
    }

    void test_disconnection_by_callable() {
        // disconnect a function pointer
        {
            sum = 0;

            Sig.connect(f1);
            Sig.connect(f2);
            Sig.connect(f2);
            Sig(1);
            assert(sum == 5);
            auto c = Sig.disconnect(&f2);
            assert(c == 2);
            Sig(1);
            assert(sum == 6);
            Sig.disconnect_all();
        }

        // disconnect a function
        {
            sum = 0;

            Sig.connect(f1);
            Sig.connect(f2);
            Sig(1);
            assert(sum == 3);
            Sig.disconnect(f1);
            Sig(1);
            assert(sum == 5);
            Sig.disconnect_all();
        }

#ifdef SIGSLOT_RTTI_ENABLED
        // disconnect by pmf
        {
            sum = 0;
            s p;

            Sig.connect(&s::f1, &p);
            Sig.connect(&s::f2, &p);
            Sig(1);
            assert(sum == 2);
            Sig.disconnect(&s::f1);
            Sig(1);
            assert(sum == 3);
            Sig.disconnect_all();
        }

        // disconnect by function object
        {
            sum = 0;

            Sig.connect(o1{});
            Sig.connect(o2{});
            Sig(1);
            assert(sum == 2);
            Sig.disconnect(o1{});
            Sig(1);
            assert(sum == 3);
            Sig.disconnect_all();
        }

        // disconnect by lambda
        {
            sum = 0;
            auto l1 = [&](int i) { sum += i; };
            auto l2 = [&](int i) { sum += 2*i; };
            Sig.connect(l1);
            Sig.connect(l2);
            Sig(1);
            assert(sum == 3);
            Sig.disconnect(l1);
            Sig(1);
            assert(sum == 5);
            Sig.disconnect_all();
        }
#endif
    }

    void test_disconnection_by_object() {
        // disconnect by pointer
        {
            sum = 0;
            s p1, p2;

            Sig.connect(&s::f1, &p1);
            Sig.connect(&s::f2, &p2);
            Sig(1);
            assert(sum == 2);
            Sig.disconnect(&p1);
            Sig(1);
            assert(sum == 3);
            Sig.disconnect_all();
        }

        // disconnect by shared pointer
        {
            sum = 0;
            auto p1 = std::make_shared<s>();
            s p2;

            Sig.connect(&s::f1, p1);
            Sig.connect(&s::f2, &p2);
            Sig(1);
            assert(sum == 2);
            Sig.disconnect(p1);
            Sig(1);
            assert(sum == 3);
            Sig.disconnect_all();
        }
    }

    void test_disconnection_by_object_and_pmf() {
        // disconnect by pointer
        {
            sum = 0;
            s p1, p2;

            Sig.connect(&s::f1, &p1);
            Sig.connect(&s::f1, &p2);
            Sig.connect(&s::f2, &p1);
            Sig.connect(&s::f2, &p2);
            Sig(1);
            assert(sum == 4);
            Sig.disconnect(&s::f1, &p2);
            Sig(1);
            assert(sum == 7);
            Sig.disconnect_all();
        }

        // disconnect by shared pointer
        {
            sum = 0;
            auto p1 = std::make_shared<s>();
            auto p2 = std::make_shared<s>();

            Sig.connect(&s::f1, p1);
            Sig.connect(&s::f1, p2);
            Sig.connect(&s::f2, p1);
            Sig.connect(&s::f2, p2);
            Sig(1);
            assert(sum == 4);
            Sig.disconnect(&s::f1, p2);
            Sig(1);
            assert(sum == 7);
            Sig.disconnect_all();
        }

        // disconnect by tracker
        {
            sum = 0;

            auto t = std::make_shared<bool>();
            Sig.connect(f1);
            Sig.connect(f2);
            Sig.connect(f1, t);
            Sig.connect(f2, t);
            Sig(1);
            assert(sum == 6);
            Sig.disconnect(f2, t);
            Sig(1);
            assert(sum == 10);
            Sig.disconnect_all();
        }
    }

    void test_scoped_connection() {
        sum = 0;

        {
            auto sc1 = Sig.connect_scoped(f1);
            Sig(1);
            assert(sum == 1);

            auto sc2 = Sig.connect_scoped(f2);
            Sig(1);
            assert(sum == 4);
        }

        Sig(1);
        assert(sum == 4);

        sum = 0;

        {
            sigslot::scoped_connection sc1 = Sig.connect(f1);
            Sig(1);
            assert(sum == 1);

            auto sc2 = Sig.connect_scoped(f2);
            Sig(1);
            assert(sum == 4);
        }

        Sig(1);
        assert(sum == 4);
    }

    void test_connection_blocking() {
        sum = 0;

        auto c1 = Sig.connect(f1);
        Sig.connect(f2);
        Sig(1);
        assert(sum == 3);

        c1.block();
        Sig(1);
        assert(sum == 5);

        c1.unblock();
        Sig(1);
        assert(sum == 8);
        Sig.disconnect_all();
    }

    void test_connection_blocker() {
        sum = 0;
        sigslot::signal<int> sig;

        auto c1 = Sig.connect(f1);
        Sig.connect(f2);
        Sig(1);
        assert(sum == 3);

        {
            auto cb = c1.blocker();
            Sig(1);
            assert(sum == 5);
        }

        Sig(1);
        assert(sum == 8);
        Sig.disconnect_all();
    }

    void test_signal_blocking() {
        sum = 0;

        Sig.connect(f1);
        Sig.connect(f2);
        Sig(1);
        assert(sum == 3);

        Sig.block();
        Sig(1);
        assert(sum == 3);

        Sig.unblock();
        Sig(1);
        assert(sum == 6);
        Sig.disconnect_all();
    }

    void test_all_disconnection() {
        sum = 0;

        Sig.connect(f1);
        Sig.connect(f2);
        Sig(1);
        assert(sum == 3);

        Sig.disconnect_all();
        Sig(1);
        assert(sum == 3);
    }

    void test_connection_copying_moving() {
        sum = 0;

        auto sc1 = Sig.connect(f1);
        auto sc2 = Sig.connect(f2);

        auto sc3 = sc1;
        auto sc4{sc2};

        auto sc5 = std::move(sc3);
        auto sc6{std::move(sc4)};

        Sig(1);
        assert(sum == 3);

        sc5.block();
        Sig(1);
        assert(sum == 5);

        sc1.unblock();
        Sig(1);
        assert(sum == 8);

        sc6.disconnect();
        Sig(1);
        assert(sum == 9);
        Sig.disconnect_all();
    }

    void test_scoped_connection_moving() {
        sum = 0;

        {
            auto sc1 = Sig.connect_scoped(f1);
            Sig(1);
            assert(sum == 1);

            auto sc2 = Sig.connect_scoped(f2);
            Sig(1);
            assert(sum == 4);

            auto sc3 = std::move(sc1);
            Sig(1);
            assert(sum == 7);

            auto sc4{std::move(sc2)};
            Sig(1);
            assert(sum == 10);
        }

        Sig(1);
        assert(sum == 10);
    }

    void test_signal_moving() {
        sum = 0;

        Sig.connect(f1);
        Sig.connect(f2);

        Sig(1);
        assert(sum == 3);

        auto sig2 = std::move(Sig);
        sig2(1);
        assert(sum == 6);

        auto sig3 = std::move(sig2);
        sig3(1);
        assert(sum == 9);
    }
};

int main() {
    owner o;

    o.test_free_connection();
    o.test_static_connection();
    o.test_pmf_connection();
    o.test_const_pmf_connection();
    o.test_function_object_connection();
    o.test_overloaded_function_object_connection();
    o.test_lambda_connection();
    o.test_generic_lambda_connection();
    o.test_lvalue_emission();
    o.test_compatible_args();
    o.test_mutation();
    o.test_disconnection();
    o.test_disconnection_by_callable();
    o.test_disconnection_by_object();
    o.test_disconnection_by_object_and_pmf();
    o.test_scoped_connection();
    o.test_connection_blocker();
    o.test_connection_blocking();
    o.test_signal_blocking();
    o.test_all_disconnection();
    o.test_connection_copying_moving();
    o.test_scoped_connection_moving();
    o.test_slot_count();
    o.test_signal_moving();
    return 0;
}
