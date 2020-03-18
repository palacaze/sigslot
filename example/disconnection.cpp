#include <sigslot/signal.hpp>
#include <iostream>
#include <string>

static int i = 0;

void f1() { i += 1; }
void f2() { i += 1; }

struct s {
    void m1() { i += 1; }
    void m2() { i += 1; }
    void m3() { i += 1; }
};

struct o {
    void operator()() { i += 1; }
};

int main() {
    sigslot::signal<> sig;
    s s1;
    auto s2 = std::make_shared<s>();

    auto lbd = [&] { i += 1; };

    sig.connect(f1);           // #1
    sig.connect(f2);           // #2
    sig.connect(&s::m1, &s1);  // #3
    sig.connect(&s::m2, &s1);  // #4
    sig.connect(&s::m3, &s1);  // #5
    sig.connect(&s::m1, s2);   // #6
    sig.connect(&s::m2, s2);   // #7
    sig.connect(o{});          // #8
    sig.connect(lbd);          // #9

    sig();  // i == 9
    std::cout << "i = " << i << std::endl;

    sig.disconnect(f2);              // #2 is removed
    sig.disconnect(&s::m1);          // #3 and #6 are removed
    sig.disconnect(o{});             // #8 and is removed
//  sig.disconnect(&o::operator());  // same as the above, more efficient
    sig.disconnect(lbd);             // #9 and is removed
    sig.disconnect(s2);              // #7 is removed
    sig.disconnect(&s::m3, &s1);     // #5 is removed, not #4

    sig();  // i == 11
    std::cout << "i = " << i << std::endl;

    sig.disconnect_all();         // remove all remaining slots
    return 0;
}

