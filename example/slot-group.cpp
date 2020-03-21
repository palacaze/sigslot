#include <sigslot/signal.hpp>
#include <functional>
#include <iostream>
#include <vector>

static auto printer(std::string pos) {
    return [pos=std::move(pos)] (std::string s, int i) {
        std::cout << pos << " to print " << s << " and " << i << std::endl;
    };
}

int main() {
    sigslot::signal<std::string, int> sig;

    sig.connect(printer("Second"), 1);
    sig.connect(printer("Third"), 2);
    sig.connect(printer("First"), 0);
    sig("bar", 1);

    return 0;
}
