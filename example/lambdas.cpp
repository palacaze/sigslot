#include <sigslot/signal.hpp>
#include <cstdio>

/*
 * This example is meant to test compilation time as well as object size when a
 * lot of unique callables get connected to a signal.
 * This example motivated the introduction of a SOCUTE_REDUCE_COMPILE_TIME cmake option.
 */

#define LAMBDA4() sig.connect([](int) { puts("lambda\n"); });
#define LAMBDA3() LAMBDA4() LAMBDA4() LAMBDA4() LAMBDA4() LAMBDA4()
#define LAMBDA2() LAMBDA3() LAMBDA3() LAMBDA3() LAMBDA3() LAMBDA3()
#define LAMBDA1() LAMBDA2() LAMBDA2() LAMBDA2() LAMBDA2() LAMBDA2()
#define LAMBDAS() LAMBDA1() LAMBDA1() LAMBDA1() LAMBDA1() LAMBDA1()

int main() {
    sigslot::signal<int> sig;

    // connect a bunch of lambdas
    LAMBDAS()
    return 0;
}
