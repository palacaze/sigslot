#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// A program that computes the function pointer size and uniqueness for a variety
// of cases.

void fun() {}

struct b1 {
    virtual ~b1() = default;
    static void sm() {}
    void m() {}
    virtual void vm() {}
};

struct b2 {
    virtual ~b2() = default;
    static void sm() {}
    void m() {}
    virtual void vm() {}
};

struct c {
    virtual ~c() = default;
    virtual void w() {}
};

struct d : b1 {
    static void sm() {}
    void m() {}
    void vm() override {}
};

struct e : b1, c {
    static void sm() {}
    void m() {}
    void vm() override{}
};

template <typename T>
union sizer {
    T t;
    char data[sizeof(T)];
};

template <typename T>
std::string ptr_string(const T&t) {
    sizer<T> ss;
    std::uninitialized_fill(std::begin(ss.data), std::end(ss.data), '\0');
    ss.t = t;
    std::string addr;
    for (char i : ss.data) {
        char b[] = "00";
        std::snprintf(b, 3, "%02X", static_cast<unsigned char>(i));
        addr += b;
    }

    // shorten string
    while (addr.size() >= 2) {
        auto si = addr.size();
        if (addr[si-1] == '0' && addr[si-2] == '0') {
            addr.pop_back();
            addr.pop_back();
        } else {
            break;
        }
    }
    return addr;
}

template <typename T>
std::string print(std::string name, const T&t) {
    auto addr = ptr_string(t);
    std::cout << name << "\t" << sizeof(t) << "\t0x" << addr << std::endl;
    return addr;
}

int main(int, char **) {
    std::vector<std::string> addrs;

    addrs.push_back(print("fun", &fun));
    addrs.push_back(print("&b1::sm", &b1::sm));
    addrs.push_back(print("&b1::m", &b1::m));
    addrs.push_back(print("&b1::vm", &b1::vm));
    addrs.push_back(print("&b2::sm", &b2::sm));
    addrs.push_back(print("&b2::m", &b2::m));
    addrs.push_back(print("&b2::vm", &b2::vm));
    addrs.push_back(print("&d::sm", &d::sm));
    addrs.push_back(print("&d::m", &d::m));
    addrs.push_back(print("&d::vm", &d::vm));
    addrs.push_back(print("&e::sm", &e::sm));
    addrs.push_back(print("&e::m", &e::m));
    addrs.push_back(print("&e::vm", &e::vm));

    std::sort(addrs.begin(), addrs.end());
    auto last = std::unique(addrs.begin(), addrs.end());
    std::cout << "Address duplicates: "
              << std::distance(last, addrs.end()) << std::endl;
    return 0;
}
