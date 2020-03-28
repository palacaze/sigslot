#include "test-common.h"
#include <sigslot/adapter/qt.hpp>
#include <sigslot/signal.hpp>
#include <cassert>

static int sum = 0;

void f1(int i) { sum += i; }
struct o1 { void operator()(int i) { sum += 2*i; } };

struct s {
    void f1(int i) { sum += i; }
    void f2(int i) const { sum += 2*i; }
};

struct dummy {};

class MyObject : public QObject {
    Q_OBJECT

public:
    MyObject(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void addSum(int i) const { sum += i; }
};

#include "qt-tracking.moc"

void test_track_shared() {
    sum = 0;
    sigslot::signal<int> sig;

    auto s1 = QSharedPointer<s>::create();
    sig.connect(&s::f1, s1);

    auto s2 = QSharedPointer<s>::create();
    auto w2 = s2.toWeakRef();
    sig.connect(&s::f2, w2);

    sig(1);
    assert(sum == 3);

    s1.clear();
    sig(1);
    assert(sum == 5);

    s2.clear();
    sig(1);
    assert(sum == 5);
}

void test_track_shared_other() {
    sum = 0;
    sigslot::signal<int> sig;

    auto d1 = QSharedPointer<dummy>::create();
    sig.connect(f1, d1);

    auto d2 = QSharedPointer<dummy>::create();
    auto w2 = d2.toWeakRef();
    sig.connect(o1(), w2);

    sig(1);
    assert(sum == 3);

    d1.reset();
    sig(1);
    assert(sum == 5);

    d2.reset();
    sig(1);
    assert(sum == 5);
}

void test_track_qobject() {
    sum = 0;
    sigslot::signal<int> sig;

    {
        MyObject o;
        sig.connect(&MyObject::addSum, &o);

        sig(1);
        assert(sum == 1);
    }

    sig(1);
    assert(sum == 1);
}

void test_track_qobject_other() {
    sum = 0;
    sigslot::signal<int> sig;

    {
        MyObject o;
        sig.connect(f1, &o);

        sig(1);
        assert(sum == 1);
    }

    sig(1);
    assert(sum == 1);
}

int main() {
    test_track_shared();
    test_track_shared_other();
    test_track_qobject();
    test_track_qobject_other();
    return 0;
}
