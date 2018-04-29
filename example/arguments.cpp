#include <sigslot/signal.hpp>
#include <iostream>
#include <string>

struct foo {
    // Notice how we accept a double as first argument here
    // This is fine because float is convertible to double
    void bar(float d, int i, bool b, std::string &s) {
        s = b ? std::to_string(i) : std::to_string(d);
    }
};

// Function objects can cope with default arguments and overloading.
// It does not work with static and member functions.
struct obj {
    void operator()(float, int, bool, std::string &, int = 0) {
        std::cout << "I was here\n";
    }

    void operator()() {}
};

// A generic function object that deals with any input argument
struct printer {
    template <typename T, typename... Ts>
    void operator()(T a, Ts && ...args) const {
       std::cout << a;
        (void)std::initializer_list<int>{
            ((void)(std::cout << " " << std::forward<Ts>(args)), 1)...
        };
        std::cout << "\n";
    }
};

int main() {
    // declare a signal with float, int, bool and string& arguments
    sigslot::signal<float, int, bool, std::string&> sig;

    // a Generic lambda that prints its arguments to stdout
    auto lambda_printer = [] (auto a, auto && ...args) {
        std::cout << a;
        (void)std::initializer_list<int>{
            ((void)(std::cout << " " << args), 1)...
        };
        std::cout << "\n";
    };

    // connect the slots
    foo ff;
    sig.connect(printer());
    sig.connect(&foo::bar, &ff);
    sig.connect(lambda_printer);
    sig.connect(obj());

    float f = 1.f;
    short i = 2;
    std::string s = "0";

    // emit a signal
    sig(f, i, false, s);
    sig(f, i, true, s);

    return 0;
}

