#include "test-common.h"
#include <type_traits>
#include <sigslot/signal.hpp>

using namespace sigslot::trait;

void f1(int, char, float) {}
void f2(int, char, float) noexcept {}

void f(int) {}
void f(int, char, float) {}

struct oo {
    void operator()(int) {}
    void operator()(int, char, float) {}
};

struct s {
    static void s1(int, char, float) {}
    static void s2(int, char, float) noexcept {}

    void f1(int, char, float) {}
    void f2(int, char, float) const {}
    void f3(int, char, float) volatile {}
    void f4(int, char, float) const volatile {}
    void f5(int, char, float) noexcept {}
    void f6(int, char, float) const noexcept {}
    void f7(int, char, float) volatile noexcept {}
    void f8(int, char, float) const volatile noexcept {}
};

struct o1 { void operator()(int, char, float) {} };
struct o2 { void operator()(int, char, float) const {} };
struct o3 { void operator()(int, char, float) volatile {} };
struct o4 { void operator()(int, char, float) const volatile {} };
struct o5 { void operator()(int, char, float) noexcept {} };
struct o6 { void operator()(int, char, float) const noexcept {} };
struct o7 { void operator()(int, char, float) volatile noexcept {} };
struct o8 { void operator()(int, char, float) const volatile noexcept {} };

static_assert(Callable<decltype(f1), int, char, float>, "");
static_assert(Callable<decltype(f2), int, char, float>, "");
static_assert(Callable<decltype(&s::s1), int, char, float>, "");
static_assert(Callable<decltype(&s::s2), int, char, float>, "");
static_assert(Callable<oo, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f1), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f2), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f3), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f4), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f5), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f6), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f7), s*, int, char, float>, "");
static_assert(MemberCallable<decltype(&s::f8), s*, int, char, float>, "");
static_assert(Callable<o1, int, char, float>, "");
static_assert(Callable<o2, int, char, float>, "");
static_assert(Callable<o3, int, char, float>, "");
static_assert(Callable<o4, int, char, float>, "");
static_assert(Callable<o5, int, char, float>, "");
static_assert(Callable<o6, int, char, float>, "");
static_assert(Callable<o7, int, char, float>, "");
static_assert(Callable<o8, int, char, float>, "");

int main() {
    auto l1 = [](int, char, float) {};
    auto l2 = [&](int, char, float) mutable {};
    auto l3 = [&](auto...) mutable {};

    static_assert(Callable<decltype(l1), int, char, float>, "");
    static_assert(Callable<decltype(l2), int, char, float>, "");
    static_assert(Callable<decltype(l3), int, char, float>, "");

    return 0;
}

