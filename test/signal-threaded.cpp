#include "test-common.h"
#include <sigslot/signal.hpp>
#include <thread>
#include <atomic>
#include <cassert>
#include <array>

static std::atomic<std::int64_t> sum{0};

static void f(int i) { sum += i; }
static void f1(int i) { sum += i; }
static void f2(int i) { sum += i; }
static void f3(int i) { sum += i; }

static void emit_many(sigslot::signal<int> &sig) {
    for (int i = 0; i < 10000; ++i)
        sig(1);
}

static void connect_emit(sigslot::signal<int> &sig) {
    for (int i = 0; i < 100; ++i) {
        auto s = sig.connect_scoped(f);
        for (int j = 0; j < 100; ++j)
            sig(1);
    }
}

static void connect_cross(sigslot::signal<int> &s1,
                          sigslot::signal<int> &s2,
                          std::atomic<int> &go)
{
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

    sigslot::signal<int> sig;

    std::array<std::thread, 10> threads;
    for (auto &t : threads)
        t = std::thread(connect_emit, std::ref(sig));

    for (auto &t : threads)
        t.join();
}

static void test_threaded_emission() {
    sum = 0;

    sigslot::signal<int> sig;
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

    sigslot::signal<int> sig1;
    sigslot::signal<int> sig2;

    std::atomic<int> go{0};

    std::thread t1(connect_cross, std::ref(sig1), std::ref(sig2), std::ref(go));
    std::thread t2(connect_cross, std::ref(sig2), std::ref(sig1), std::ref(go));

    while (go != 2)
        std::this_thread::yield();
    go++;

    t1.join();
    t2.join();

    assert(sum == std::int64_t(1000000000000ll));
}

// test what happens when more than one thread attempt disconnection
static void test_threaded_misc() {
    sum = 0;
    sigslot::signal<int> sig;
    std::atomic<bool> run{true};

    auto emitter = [&] {
        while (run) {
            sig(1);
        }
    };

    auto conn = [&] {
        while (run) {
            for (int i = 0; i < 10; ++i) {
                sig.connect(f1);
                sig.connect(f2);
                sig.connect(f3);
            }
        }
    };

    auto disconn = [&] {
        unsigned int i = 0;
        while (run) {
            if (i == 0)
                sig.disconnect(f1);
            else if (i == 1)
                sig.disconnect(f2);
            else
                sig.disconnect(f3);
            i++;
            i = i % 3;
        }
    };

    std::array<std::thread, 20> emitters;
    std::array<std::thread, 20> conns;
    std::array<std::thread, 20> disconns;

    for (auto &t :conns)
        t = std::thread(conn);
    for (auto &t : emitters)
        t = std::thread(emitter);
    for (auto &t : disconns)
        t = std::thread(disconn);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    run = false;

    for (auto &t : emitters)
        t.join();
    for (auto &t : disconns)
        t.join();
    for (auto &t : conns)
        t.join();
}

int main() {
    test_threaded_emission();
    test_threaded_mix();
    test_threaded_crossed();
    test_threaded_misc();

    return 0;
}
