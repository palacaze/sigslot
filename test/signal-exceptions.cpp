#include <cassert>
#include <sigslot/signal.hpp>

class TestingClass final {
public:
    TestingClass(bool should_throw, std::atomic_int &cumulative) : m_should_throw(should_throw),
                                                                   m_cumulative(cumulative) {
    }

    void function(int x) const {
        m_cumulative += x;
        if (m_should_throw) {
            throw std::runtime_error("Throwing variant");
        }
    }

private:
    bool m_should_throw;
    std::atomic_int &m_cumulative;
};

void test_plain_exceptions() {
    constexpr int k_emitted_value{3};

    std::atomic_int cumulative{0};

    TestingClass t1{true, cumulative};
    TestingClass t2{false, cumulative};
    TestingClass t3{true, cumulative};

    sigslot::signal<int> signal;

    signal.connect(&TestingClass::function, &t1, 1);
    signal.connect(&TestingClass::function, &t2, 2);
    signal.connect(&TestingClass::function, &t3, 3);

    std::vector<std::exception_ptr> exceptions;
    try {
        exceptions = signal(k_emitted_value);
    } catch (...) {
        assert(!static_cast<bool>("Should not get exceptions outside signals!"));
    }

    assert(exceptions.size() == 2);
    assert(cumulative == (signal.slot_count() * k_emitted_value));
}

void test_connected_signal_exceptions() {
    constexpr int k_emitted_value{3};

    std::atomic_int cumulative{0};
    TestingClass tc{true, cumulative};

    // Create two signals
    sigslot::signal<int> signal1;
    sigslot::signal<int> signal2;

    // Connect the second signal with the first one
    sigslot::connect(signal1, signal2);

    // Connect the TestingClass function method to the second signal
    signal2.connect(&TestingClass::function, &tc);

    std::vector<std::exception_ptr> exceptions;
    // Emit the first signal
    try {
        exceptions = signal1(k_emitted_value);
    } catch (...) {
        assert(!static_cast<bool>("Should not get exceptions outside signals!"));
    }

    assert(exceptions.size() == 1);
    assert(cumulative == k_emitted_value);
}

int main() {
    test_plain_exceptions();
    test_connected_signal_exceptions();
}
