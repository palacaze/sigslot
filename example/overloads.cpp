#include <sigslot/signal.hpp>

template <typename... Args, typename C>
constexpr auto overload(void (C::*ptr)(Args...)) {
    return ptr;
}

template <typename... Args>
constexpr auto overload(void (*ptr)(Args...)) {
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
    sig.connect(overload<int>(&foo::bar), &ff);
    sig.connect(overload<int>(&foo::baz));
    sig.connect(overload<int>(&moo));
    sig.connect(obj());

    sig(0);

    return 0;
}

