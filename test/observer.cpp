#include "test-common.h"
#include <sigslot/signal.hpp>
#include <cassert>
#include <list>
#include <memory>
#include <vector>

struct s : ::sigslot::observer {
    ~s() override {
        this->disconnect_all();
    }

    void f1 (int &i) { ++i; }
};

struct s_st : ::sigslot::observer_st {
    void f1 (int &i) { ++i; }
};

struct s_plain {
    void f1 (int &i) { ++i; }
};

template <typename T, template <typename...> class SIG_T>
void test_observer() {
    SIG_T<int &> sig;

    // Automatic disconnect via observer inheritance
    {
        T p1;
        sig.connect(&T::f1, &p1);
        assert(sig.slot_count() == 1);

        {
            T p2;
            sig.connect(&T::f1, &p2);
            assert(sig.slot_count() == 2);
        }

        assert(sig.slot_count() == 1);
    }

    assert(sig.slot_count() == 0);

    // No automatic disconnect
    {
        s_plain p;
        sig.connect(&s_plain::f1, &p);
        assert(sig.slot_count() == 1);
    }

    assert(sig.slot_count() == 1);
}

template <typename T, template <typename...> class SIG_T>
void test_observer_signals() {
    int sum = 0;
    SIG_T<int &> sig;

    {
        T p1;
        sig.connect(&T::f1, &p1);
        sig(sum);
        assert(sum == 1);
        {
            T p2;
            sig.connect(&T::f1, &p2);
            sig(sum);
            assert(sum == 3);
        }
        sig(sum);
        assert(sum == 4);
    }

    sig(sum);
    assert(sum == 4);
}

template <typename T, template <typename...> class SIG_T>
void test_observer_signals_shared() {
    int sum = 0;
    SIG_T<int &> sig;

    {
        auto p1 = std::make_shared<T>();
        sig.connect(&T::f1, p1);
        sig(sum);
        assert(sum == 1);
        {
            auto p2 = std::make_shared<T>();
            sig.connect(&T::f1, p2);
            sig(sum);
            assert(sum == 3);
        }
        sig(sum);
        assert(sum == 4);
    }

    sig(sum);
    assert(sum == 4);
}

template <typename T, template <typename...> class SIG_T>
void test_observer_signals_list() {
    int sum = 0;
    SIG_T<int &> sig;

    {
        std::list<T> l;
        for (auto i = 0; i < 10; ++i)
        {
            l.emplace_back();
            sig.connect(&T::f1, &l.back());
        }

        assert(sig.slot_count() == 10);
        sig(sum);
        assert(sum == 10);
    }

    assert(sig.slot_count() == 0);
    sig(sum);
    assert(sum == 10);
}

template <typename T, template <typename...> class SIG_T>
void test_observer_signals_vector() {
    int sum = 0;
    SIG_T<int &> sig;

    {
        std::vector<std::unique_ptr<T>> v;
        for (auto i = 0; i < 10; ++i)
        {
            v.emplace_back(new T);
            sig.connect(&T::f1, v.back().get());
        }
        assert(sig.slot_count() == 10);
        sig(sum);
        assert(sum == 10);
    }

    assert(sig.slot_count() == 0);
    sig(sum);
    assert(sum == 10);
}

int main()
{
    test_observer<s, sigslot::signal>();
    test_observer<s_st, sigslot::signal_st>();
    test_observer_signals<s, sigslot::signal>();
    test_observer_signals<s_st, sigslot::signal_st>();
    test_observer_signals_shared<s, sigslot::signal>();
    test_observer_signals_shared<s_st, sigslot::signal_st>();
    test_observer_signals_list<s, sigslot::signal>();
    test_observer_signals_list<s_st, sigslot::signal_st>();
    test_observer_signals_vector<s, sigslot::signal>();
    test_observer_signals_vector<s_st, sigslot::signal_st>();
    return 0;
}
