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

using t = typelist<int, char, float>;

static_assert(is_callable_v<t, decltype(f1)>, "");
static_assert(is_callable_v<t, decltype(f2)>, "");
static_assert(is_callable_v<t, decltype(&s::s1)>, "");
static_assert(is_callable_v<t, decltype(&s::s2)>, "");
static_assert(is_callable_v<t, oo>, "");
static_assert(is_callable_v<t, decltype(&s::f1), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f2), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f3), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f4), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f5), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f6), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f7), s*>, "");
static_assert(is_callable_v<t, decltype(&s::f8), s*>, "");
static_assert(is_callable_v<t, o1>, "");
static_assert(is_callable_v<t, o2>, "");
static_assert(is_callable_v<t, o3>, "");
static_assert(is_callable_v<t, o4>, "");
static_assert(is_callable_v<t, o5>, "");
static_assert(is_callable_v<t, o6>, "");
static_assert(is_callable_v<t, o7>, "");
static_assert(is_callable_v<t, o8>, "");

int main() {
    auto l1 = [](int, char, float) {};
    auto l2 = [&](int, char, float) mutable {};
    auto l3 = [&](auto...) mutable {};

    static_assert(is_callable_v<t, decltype(l1)>, "");
    static_assert(is_callable_v<t, decltype(l2)>, "");
    static_assert(is_callable_v<t, decltype(l3)>, "");

    return 0;
}

