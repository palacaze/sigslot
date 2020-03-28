#include "test-common.h"
#include <sigslot/signal.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <random>

using res_container = std::vector<sigslot::group_id>;

static constexpr size_t num_groups = 100;
static constexpr size_t num_slots = 1000;

static auto pusher(int pos) {
    return [pos=std::move(pos)] (res_container &c) {
        c.push_back(pos);
    };
}

static auto adder(int v) {
    return [v=std::move(v)] (int &s) {
        s += v;
    };
}

static void test_random_groups() {
    res_container results;
    sigslot::signal<res_container&> sig;

    std::mt19937_64 gen{std::random_device()()};

    // create N groups with random ids
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::lowest());
    std::array<sigslot::group_id, num_groups> gids;
    std::generate_n(gids.begin(), num_groups, [&] { return dist(gen); });

    // create
    std::uniform_int_distribution<size_t> slots_dist { 0, num_groups-1 };

    for (size_t i = 0; i < num_slots; ++i) {
        auto gid = gids[slots_dist(gen)];
        sig.connect(pusher(gid), gid);
    }

    // signal
    sig(results);

    // check that the resulting container is sorted
    assert(std::is_sorted(results.begin(), results.end()));
}

static void test_disconnect_group() {
    int sum = 0;
    sigslot::signal<int&> sig;
    sig.connect(adder(3), 3);
    sig.connect(adder(1), 1);
    sig.connect(adder(2), 2);

    sig(sum);
    assert(sum == 6);

    sig.disconnect(2);
    sig(sum);
    assert(sum == 10);
}

int main() {
    test_random_groups();
    test_disconnect_group();
    return 0;
}
