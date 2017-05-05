#include <type_traits>
#include <tuple>
#include <sigslot/detail/traits/typelist.hpp>

using namespace pal::traits;

using t = typelist<int, char, float>;

// concatenate
static_assert(std::is_same<typelist<int, float>, concat_t<typelist<int>, typelist<float>>>::value, "");

// add and remove elements
static_assert(std::is_same<t, add_first_t<typelist<char, float>, int>>::value, "");
static_assert(std::is_same<t, add_last_t<typelist<int, char>, float>>::value, "");
static_assert(std::is_same<typelist<char, float>, rem_first_t<t>>::value, "");
static_assert(std::is_same<typelist<>, rem_first_t<typelist<>>>::value, "");
static_assert(std::is_same<typelist<int, char>, rem_last_t<t>>::value, "");
static_assert(std::is_same<typelist<>, rem_last_t<typelist<>>>::value, "");

// filter elements
template <typename T>
using not_char = std::integral_constant<bool, !std::is_same<T, char>::value>;

static_assert(std::is_same<filter_t<not_char, t>, typelist<int, float>>::value, "");

// apply typelist
template <typename... T>
struct custom_list {};

static_assert(std::is_same<apply_t<custom_list, t>, custom_list<int, char, float>>::value, "");
static_assert(std::is_same<apply_t<std::tuple, t>, std::tuple<int, char, float>>::value, "");

int main() {}

