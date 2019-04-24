#include "test-common.h"
#include <sigslot/signal.hpp>
#include <thread>
#include <atomic>
#include <cassert>
#include <array>

using namespace sigslot;

std::atomic<long int> sum{0};

static void f(int i) { sum += i; }

static void emit_many(signal<int> &sig) {
    for (int i = 0; i < 10000; ++i)
        sig(1);
}

static void connect_emit(signal<int> &sig) {
    for (int i = 0; i < 100; ++i) {
        auto s = sig.connect_scoped(f);
        for (int j = 0; j < 100; ++j)
            sig(1);
    }
}

static void connect_cross(signal<int> &s1, signal<int> &s2, std::atomic<int> &go) {
    auto cross = s1.connect([&](int i) {
        if (i & 1)
            f(i);
        else
            s2(i+1);
    });

    go++;
    while (go != 3)
        std::this_thread::yield();

    for (int i = 0; i < 1000000; ++i)
        s1(i);
}

static void test_threaded_mix() {
    sum = 0;

    signal<int> sig;

    std::array<std::thread, 10> threads;
    for (auto &t : threads)
        t = std::thread(connect_emit, std::ref(sig));

    for (auto &t : threads)
        t.join();
}

static void test_threaded_emission() {
    sum = 0;

    signal<int> sig;
    sig.connect(f);

    std::array<std::thread, 10> threads;
    for (auto &t : threads)
        t = std::thread(emit_many, std::ref(sig));

    for (auto &t : threads)
        t.join();

    assert(sum == 100000l);
}

// test for deadlocks in cross emission situation
static void test_threaded_crossed() {
    sum = 0;

    signal<int> sig1;
    signal<int> sig2;

    std::atomic<int> go{0};

    std::thread t1(connect_cross, std::ref(sig1), std::ref(sig2), std::ref(go));
    std::thread t2(connect_cross, std::ref(sig2), std::ref(sig1), std::ref(go));

    while (go != 2)
        std::this_thread::yield();
    go++;

    t1.join();
    t2.join();

    assert(sum == 1000000000000l);
}

int main() {
    test_threaded_emission();
    test_threaded_mix();
    test_threaded_crossed();
    return 0;
}
