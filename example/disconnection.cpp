#include <sigslot/signal.hpp>
#include <iostream>
#include <string>

namespace {
static int ti = 0;
} // anonymous namespace

void f1() { ti += 1; }
void f2() { ti += 1; }

struct s {
    virtual ~s() = default;
    void m1() { ti += 1; }
    void m2() { ti += 1; }
    void m3() { ti += 1; }

    virtual void v() = 0;
};

struct b {
    virtual ~b() = default;
};

struct d : s, b {
    void v() override { ti += 1; }
};

struct o {
    void operator()() { ti += 1; }
};

int main() {
    sigslot::signal<> sig;
    d s1;
    auto s2 = std::make_shared<d>();

    auto lbd = [&] { ti += 1; };

    sig.connect(f1);           // #1
    sig.connect(f2);           // #2
    sig.connect(&s::m1, &s1);  // #3
    sig.connect(&s::m2, &s1);  // #4
    sig.connect(&s::m3, &s1);  // #5
    sig.connect(&s::m1, s2);   // #6
    sig.connect(&s::m2, s2);   // #7
    sig.connect(&s::v, s2);    // #8
    sig.connect(o{});          // #0
    sig.connect(lbd);          // #10

    sig();  // i == 10
    std::cout << "i = " << ti << std::endl;

    sig.disconnect(f2);              // #2 is removed

#ifdef SIGSLOT_RTTI_ENABLED
    sig.disconnect(&s::m1);          // #3 and #6 are removed
    sig.disconnect(o{});             // #9 and is removed
//  sig.disconnect(&o::operator());  // same as the above, more efficient
    sig.disconnect(lbd);             // #10 and is removed
    sig.disconnect(&s::v);           // #8 and is removed
#endif

    sig.disconnect(s2);              // #7 is removed
    sig.disconnect(&s::m3, &s1);     // #5 is removed, not #4

    sig();  // i == 12 with RTTI, 15 without RTTI
    std::cout << "i = " << ti << std::endl;

    sig.disconnect_all();         // remove all remaining slots
    return 0;
}
