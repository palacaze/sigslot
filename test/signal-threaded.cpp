#include <sigslot/signal.hpp>
#include <thread>
#include <atomic>
#include <cassert>

using namespace pal;

std::atomic<int> sum{0};

void f(int i) { sum += i; }

void emit_many(signal<int> &sig) {
    for (int i = 0; i < 10000; ++i)
        sig(1);
}

void test_threaded_emission() {
    sum = 0;

    signal<int> sig;
    sig.connect(f);

    std::array<std::thread, 10> threads;
    for (auto &t : threads)
        t = std::thread(emit_many, std::ref(sig));

    for (auto &t : threads)
        t.join();

    assert(sum == 100000);
}

int main() {
    test_threaded_emission();
    return 0;
}

