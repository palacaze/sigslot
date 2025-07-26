#include <sigslot/signal.hpp>
#include <iostream>

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

void f() { std::cout << "free function\n"; }

struct s {
    void m() { std::cout << "member function\n"; }
    static void sm() { std::cout << "static member function\n";  }
};

struct o {
    void operator()() { std::cout << "function object\n"; }
};

void basic_member_connect() {
    s d;
    auto lambda = []() { std::cout << "lambda\n"; };

    // declare a signal instance with no arguments
    sigslot::signal<> sig;

    // sigslot::signal will connect to any callable provided it has compatible
    // arguments. Here are diverse examples
    sig.connect(f);
    sig.connect(&s::m, &d);
    sig.connect(&s::sm);
    sig.connect(o());
    sig.connect(lambda);

    // Avoid hitting bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68071
    // on old GCC compilers
#if defined(__clang__) || GCC_VERSION > 70300
    auto gen_lambda = [](auto && ... /*a*/) { std::cout << "generic lambda\n"; };
    sig.connect(gen_lambda);
#endif

    // emit a signal
    sig();
}

void basic_freestanding_connect() {
    s d;
    auto lambda = []() { std::cout << "lambda\n"; };

    // declare a signal instance with no arguments
    sigslot::signal<> sig;
    sigslot::signal<> sig2;

    // sigslot::signal will connect to any callable provided it has compatible
    // arguments. Here are diverse examples
    sigslot::connect(sig, f);
    sigslot::connect(sig, &s::m, &d);
    sigslot::connect(sig, &s::sm);
    sigslot::connect(sig, o());
    sigslot::connect(sig, lambda);
    sigslot::connect(sig, sig2);

    sigslot::connect(sig2, [] { std::cout << "Signal chaining too\n"; });

    // Avoid hitting bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68071
    // on old GCC compilers
#if defined(__clang__) || GCC_VERSION > 70300
    auto gen_lambda = [](auto && ... /*a*/) { std::cout << "generic lambda\n"; };
    sigslot::connect(sig, gen_lambda);
#endif

    // emit a signal
    sig();
}

int main() {
    basic_member_connect();
    basic_freestanding_connect();
    return 0;
}
