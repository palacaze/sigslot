#include "test-common.h"
#include <sigslot/signal.hpp>
#include <chrono>
#include <cassert>
#include <iostream>
#include <vector>

void test_signal_performance() {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    constexpr int count = 1000;
    double ref_ns = 0.;
    sigslot::signal<> sig;
    {
        std::vector<sigslot::scoped_connection> connections;
        connections.reserve(count);
        for (int i = 0; i < count; i++) {
            connections.emplace_back(sig.connect([]{}));
        }

        // Measure first signal time as reference
        const TimePoint begin = Clock::now();
        sig();
        ref_ns = double(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count());
    }

    // Measure signal after all slot were disconnected
    const TimePoint begin = Clock::now();
    sig();
    const double after_disconnection_ns = double(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count());

    // Ensure that the signal cost is not > at 10%
    const auto max_delta = 0.1;
    const auto delta = (after_disconnection_ns - ref_ns) / ref_ns;

    std::cout << "ref: " << ref_ns / 1000 << " us" << std::endl;
    std::cout << "after: " << after_disconnection_ns / 1000 << " us" << std::endl;
    std::cout << "delta: " << delta << " SU" << std::endl;

    assert(delta < max_delta);
}

int main() {
    test_signal_performance();
    return 0;
}
