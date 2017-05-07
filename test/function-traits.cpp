#include <type_traits>
#include <sigslot/detail/traits/function.hpp>

using namespace pal::traits;

void f1(int, char, float) {}
void f2(int, char, float) noexcept {}

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

template <typename C>
using r = signature<C, void, int, char, float>;
using rf = r<no_class>;
using rs = r<s>;
using t = typelist<int, char, float>;

// free function
static_assert(std::is_same<rf, sig_t<decltype(f1)>>::value, "");
static_assert(std::is_same<rf, sig_t<decltype(f2)>>::value, "");
static_assert(std::is_same<rf, sig_t<decltype(&f1)>>::value, "");
static_assert(std::is_same<rf, sig_t<decltype(&f2)>>::value, "");

// static function
static_assert(std::is_same<rf, sig_t<decltype(&s::s1)>>::value, "");
static_assert(std::is_same<rf, sig_t<decltype(&s::s2)>>::value, "");

// member function
static_assert(std::is_same<rs, sig_t<decltype(&s::f1)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f2)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f3)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f4)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f5)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f6)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f7)>>::value, "");
static_assert(std::is_same<rs, sig_t<decltype(&s::f8)>>::value, "");

// function object
static_assert(std::is_same<r<o1>, sig_t<o1>>::value, "");
static_assert(std::is_same<r<o2>, sig_t<o2>>::value, "");
static_assert(std::is_same<r<o3>, sig_t<o3>>::value, "");
static_assert(std::is_same<r<o4>, sig_t<o4>>::value, "");
static_assert(std::is_same<r<o5>, sig_t<o5>>::value, "");
static_assert(std::is_same<r<o6>, sig_t<o6>>::value, "");
static_assert(std::is_same<r<o7>, sig_t<o7>>::value, "");
static_assert(std::is_same<r<o8>, sig_t<o8>>::value, "");

// test class, argument and result types selection
static_assert(std::is_same<t, args_t<o1>>::value, "");
static_assert(std::is_same<void, return_t<o1>>::value, "");
static_assert(std::is_same<o1, class_t<o1>>::value, "");

int main() {
    auto l1 = [](int, char, float) {};
    auto l2 = [&](int, char, float) mutable {};

    // lambda, class name can't be guessed, so we only compare args and return types
    static_assert(std::is_same<t, args_t<decltype(l1)>>::value, "");
    static_assert(std::is_same<void, return_t<decltype(l1)>>::value, "");
    static_assert(std::is_same<t, args_t<decltype(l2)>>::value, "");
    static_assert(std::is_same<void, return_t<decltype(l2)>>::value, "");

    return 0;
}

