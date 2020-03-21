#include <sigslot/signal.hpp>
#include <iostream>
#include <string>

static int i = 0;

void f() {
    i += 1;
    std::cout << "i == " << i << std::endl;
}

int main() {
    sigslot::signal<> sig;

    // keep a sigslot::connection object
    auto c1 = sig.connect(f);

    // disconnection
    sig();  // i == 1
    c1.disconnect();
    sig();  // i == 1

    // scope based disconnection
    {
        sigslot::scoped_connection sc = sig.connect(f);
        sig();  // i == 2
    }

    sig();  // i == 2;


    // connection blocking
    auto c2 = sig.connect(f);
    sig();  // i == 3
    c2.block();
    sig();  // i == 3
    c2.unblock();
    sig();  // i == 4

    c2.disconnect();

    // extended connection
    auto f2 = [](auto &con) {
        f();                // execute one
        con.disconnect();   // then disconnects
    };

    sig.connect_extended(f2);
    sig();  // i == 5
    sig();  // i == 5 because f2 was disconnected

    return 0;
}
