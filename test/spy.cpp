#include "test-common.h"
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <sigslot/signal.hpp>

/*
 * This test mocks a very simple QSignalSpy in threaded contexts.
 * It blocks the main thread, so it cannot be used in conjunction with an
 * event loop.
 */

namespace sigslot {

template <typename... T>
class spy : public std::vector<std::tuple<T...>> {
public:
    explicit spy(sigslot::signal<T...> &sig)
        : m_sig(std::addressof(sig))
    {
        init();
    }

    spy(const spy &) = delete;
    spy & operator=(const spy &) = delete;
    spy(spy &&o)
        : m_sig(o.m_sig)
    {
        o.m_conn.disconnect();
        init();
    }

    auto& signal() { return m_sig; }

    bool wait(unsigned int timeout_ms) {
        const size_t cur_size = this->size();
        std::thread([=] {
            std::unique_lock<std::mutex> lck(m_mut);
            m_cv.wait_for(lck, std::chrono::milliseconds(timeout_ms), [&] {
                return this->size() != cur_size;
            });
        }).join();
        return cur_size != this->size();
    }

private:
    void init() {
        m_conn = m_sig->connect([this] (auto &&... t) {
             this->emplace_back(std::forward<decltype(t)>(t)...);
             m_cv.notify_one();
        });
    }

    sigslot::signal<T...> *m_sig;
    scoped_connection m_conn;
    std::mutex m_mut;
    std::condition_variable m_cv;
};

template <typename... T>
auto make_spy(sigslot::signal<T...> &s) {
    return spy<T...>(s);
}

}  // namespace sigslot

namespace {

void sleep_for(unsigned ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool test_spy(unsigned timeout_ms) {
    sigslot::signal<int> sig_int;
    sigslot::signal<std::string> sig_str;
    sig_int.connect([&] (int i) {
        sig_str(std::to_string(i));
    });

    std::thread thread([&] {
        sleep_for(100);
        sig_int(2);
        sleep_for(200);
    });

    auto s = sigslot::make_spy(sig_str);
    bool ok = s.wait(timeout_ms) && s.size() == 1;

    if (thread.joinable()) {
        thread.join();
    }

    return ok;
}

void test_spy_ok() {
    bool r = test_spy(200);
    assert(r);
}

void test_spy_ko() {
    bool r = test_spy(50);
    assert(!r);
}

}  // anonymous namespace

int main(int, char**) {
    test_spy_ok();
    test_spy_ko();
}
