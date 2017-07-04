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

