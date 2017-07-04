# Sigslot, a signal-slot library

Sigslot is a header-only, thread safe implementation of signal-slots for C++.

## Features

The main goal was to replace Boost.Signals2.

Apart from the usual features, it offers thread safety, (extensible through ADL) object lifetime tracking for automatic slot disconnection, Boost.Signals2 style RAII connection management, reasonable performance and a simple and straightforward implementation.

Sigslot is unit-tested, should be reliable and stable enough to replace Boost Signals2 (it works for me). I have done some simple benchmarking (fork of the [Signal-Slot Benchmarks](https://github.com/palacaze/signal-slot-benchmarks) project) and it seems to be doing fine performance-wise, though I can't vouch for these benchmarks' relevance.

Many implementations allow signal return types, Sigslot does not because I have no use for them. Although it would be a simple enough feature to add, it would most certainly double the source code as per the need of void return type specializations everywhere. If I can be convinced of otherwise I may change my mind later on.

## Installation

No compilation or installation is required, just include `sigslot/signal.hpp` and use it. Sigslot currently depends on a C++14 compliant compiler, but if need arises it may be retrofitted to C++11. It is known to work with Clang 4.0 and Gcc 5.0+.

A CMake build file is supplied for installation purpose and generating a CMake import module. It is also required for examples and tests, which optionally depend on Qt5 and Boost for adapters unit tests.

Installation may be done using the following instructions from the root directory:

```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/local
cmake --build . --target install

# If you want to compile examples:
cmake --build . --target examples

# And compile/execute unit tests:
cmake --build . --target tests
```

## Documentation

Sigslot implements the signal-slot construct popular in UI frameworks, making it easy to use the observer pattern or event-based programming. The main entry point of the library is the `sigslot::signal<T...>` class template.

A signal is an object that can emit typed notifications, really values parametrized after the signal class template parameters, and register any number of notification handlers (callables) of compatible argument types to be executed with the values supplied whenever a signal emission happens. In signal-slot parlance this is called connecting a slot to a signal, where a "slot" represents a callable instance and a "connection" can be thought of as a conceptual link from signal to slot.

All the snippets presented below are available in compilable source code form in the example subdirectory.

### Basic usage

Here is a first example that showcases the most basic features of the library.

We first declare parameter-free signal `sig`, then we proceed to connect several slots and at last emit a signal which triggers the invocation of every slot callable connected beforehand. Notice how The library copes with diverse forms of callables.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>

void f() { std::cout << "free function\n"; }

struct s {
    void m() { std::cout << "member function\n"; }
    static void sm() { std::cout << "static member function\n";  }
};

struct o {
    void operator()() { std::cout << "function object\n"; }
};

int main() {
    s d;
    auto lambda = []() { std::cout << "lambda\n"; };
    auto gen_lambda = [](auto &&) { std::cout << "generic lambda\n"; };

    // declare a signal instance with no arguments
    sigslot::signal<> sig;

    // connect slots
    sig.connect(f);
    sig.connect(&s::m, &d);
    sig.connect(&s::sm);
    sig.connect(o());
    sig.connect(lambda);
    sig.connect(gen_lambda);

    // emit a signal
    sig();
}
```

The slot invocation order when emitting a signal is unspecified, please do not rely on it being always the same.

### Signal with arguments

That first example was simple but not so useful, let us move on to a signal that emits values instead.
A signal can emit any number of arguments, below.

```cpp
#include <sigslot/signal.hpp>
#include <iostream>
#include <string>

struct foo {
    // Notice how we accept a double as first argument here.
    // This is fine because float is convertible to double.
    // 's' is a reference and can thus be modified.
    void bar(double d, int i, bool b, std::string &s) {
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

int main() {
    // declare a signal with float, int, bool and string& arguments
    sigslot::signal<float, int, bool, std::string&> sig;

    // a generic lambda that prints its arguments to stdout
    auto printer = [] (auto a, auto ...args) {
        std::cout << a;
        (void)std::initializer_list<int>{
            ((void)(std::cout << " " << args), 1)...
        };
        std::cout << "\n";
    };

    // connect the slots
    foo ff;
    sig.connect(printer);
    sig.connect(&foo::bar, &ff);
    sig.connect(obj());

    float f = 1.f;
    short i = 2;  // convertible to int
    std::string s = "0";

    // emit a signal
    sig(f, i, false, s);
    sig(f, i, true, s);
}
```

As shown, slots arguments types don't need to be strictly identical to the signal template parameters, being convertible-from is fine. Generic arguments are fine too, as shown with the `printer` generic lambda (which could have been written as a function template too).

Right now there are two limitations that I can think of with respect to callable handling: default arguments and function overloading. Both are handled correctly in the case of function objects, but will fail to compile with static and member functions, for different but related reasons.

#### Coping with overloaded functions

Consider the following piece of code:

```cpp
struct foo {
    void bar(double d);
    void bar();
};
```

What should &foo::bar refer to? As per overloading, this pointer over member function does not map to a unique symbol, so the compiler won't be able to pick the right symbol. One way of resolving the right symbol is to explicitly cast the function pointer to the right function type. Here is an example that does just that using a little helper tool for a lighter syntax (In fact I will probably add this to the library soon).

```cpp
#include <sigslot/signal.hpp>

template <typename... Args, typename C>
constexpr auto overload_of(void (C::*ptr)(Args...)) {
    return ptr;
}

template <typename... Args>
constexpr auto overload_of(void (*ptr)(Args...)) {
    return ptr;
}

struct obj {
    void operator()(int) const {}
    void operator()() {}
};

struct foo {
    void bar(int) {}
    void bar() {}

    static void baz(int) {}
    static void baz() {}
};

void moo(int) {}
void moo() {}

int main() {
    sigslot::signal<int> sig;

    // connect the slots, casting to the right overload if necessary
    foo ff;
    sig.connect(overload_of<int>(&foo::bar), &ff);
    sig.connect(overload_of<int>(&foo::baz));
    sig.connect(overload_of<int>(&moo));
    sig.connect(obj());

    sig(0);

    return 0;
}
```

#### Coping with function with default arguments

Default arguments are not part of the function type signature, and can be redefined, so they are really difficult to deal with. When connecting a slot to a signal, the library determines if the supplied callable can be invoked with the signal argument types, but at this point the existence of default function arguments it unknown so there may be a mismatch in the number of arguments.

The simplest way of dealing with this use case would be to create a bind adapter, in fact we can even make it quite generic like so (I know, booo, a macro!):

```cpp
#include <sigslot/signal.hpp>

#define ADAPT(func) \
    [=](auto && ...a) { (func)(std::forward<decltype(a)>(a)...); }

void foo(int &i, int b = 1) {
    i += b;
}

int main() {
    int i = 0;

    // fine, all the arguments are handled
    sigslot::signal<int&, int> sig1;
    sig1.connect(foo);
    sig1(i, 2);

    // must wrap in an adapter
    i = 0;
    sigslot::signal<int&> sig2;
    sig2.connect(ADAPT(foo));
    sig2(i);

    return 0;
}
```

### Connection management

### Thread safety

