#include "test-common.h"
#include <sigslot/signal.hpp>
#include <cassert>
#include <list>
#include <vector>

struct s : ::sigslot::observer
{
    void f1 (int& i) { ++i; }
};

struct s2
{
    void f1 (int& i) { ++i; }
};

void test_observer() {
    sigslot::signal<int&> sig;

    // Automatic disconnect via observer inheritance
    {
        s p;
        sig.connect(&s::f1, &p);
        assert(sig.slot_count() == 1);

        {
            s p;
            sig.connect(&s::f1, &p);
            assert(sig.slot_count() == 2);
        }

        assert(sig.slot_count() == 1);
    }

    assert(sig.slot_count() == 0);

    // No automatic disconnect
    {
        s2 p;
        sig.connect(&s2::f1, &p);
        assert(sig.slot_count() == 1);
    }

    assert(sig.slot_count() == 1);
}

void test_observer_signals() {
    int sum = 0;
    sigslot::signal<int&> sig;

    {
        s p;
        sig.connect(&s::f1, &p);
        sig(sum);
        assert(sum == 1);
        {
            s p;
            sig.connect(&s::f1, &p);
            sig(sum);
            assert(sum == 3);
        }
        sig(sum);
        assert(sum == 4);
    }
    sig(sum);
    assert(sum == 4);
}

void test_observer_signals_list() {
    int sum = 0;
    sigslot::signal<int&> sig;

    {
        std::list<s> l;
        for (auto i = 0; i < 10; ++i) {
            l.emplace_back();
            sig.connect(&s::f1, &l.back());
        }
        assert(sig.slot_count() == 10);
        sig(sum);
        assert(sum == 10);
    }
    assert(sig.slot_count() == 0);
    sig(sum);
    assert(sum == 10);
}

void test_observer_signals_vector() {
    int sum = 0;
    sigslot::signal<int&> sig;

    {
        std::vector<std::unique_ptr<s>> v;
        for (auto i = 0; i < 10; ++i) {
            v.emplace_back(new s);
            sig.connect(&s::f1, v.back().get());
        }
        assert(sig.slot_count() == 10);
        sig(sum);
        assert(sum == 10);
    }
        assert(sig.slot_count() == 0);
    sig(sum);
    assert(sum == 10);
}

int main() {
    test_observer();
    test_observer_signals();
    test_observer_signals_list();
    test_observer_signals_vector();
    return 0;
}
