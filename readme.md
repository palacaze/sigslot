# Sigslot, a signal-slot library

Sigslot is a header-only, thread safe implementation of signal-slots for C++.

# Features

The main goal was to replace Boost.Signals2.

Apart from the usual features, it allows (extensible through ADL) object lifetime tracking for automatic slot disconnection, Boost.Signals2 style RAII connection management and a simple and straightforward implementation.

Sigslot is unit-tested, but has not been benchmarked in any way yet

## Installation

No compilation or installation is required, just include `sigslot/signal.hpp` and use it.

A cmake build file is supplied for installation anyway, as a means of installing include files and a cmake module in a proper destination. It is also required for tests, which optionally depend on Qt5 and Boost for adapters unit tests.

Installation would be done that way, from the root directory:

```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/local
cmake --build . --target install
```

And tests:

```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/local
cmake --build . --target tests
```

## Examples

TBD
