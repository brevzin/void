#include <void/void.h>
#include "catch.hpp"
#include <cxxabi.h>

template <class T> struct type_t { };
template <class T> constexpr type_t<T> type{};

template <class T, class U>
constexpr std::bool_constant<std::is_same_v<T, U>> operator==(type_t<T>, type_t<U>) {
    return {};
}

template <class T, class U>
constexpr std::bool_constant<!std::is_same_v<T, U>> operator!=(type_t<T>, type_t<U>) {
    return {};
}

namespace Catch {
    template <class T>
    struct StringMaker<type_t<T>> {
        static auto convert(type_t<T>) -> std::string {
            char const* name = typeid(T).name();

            std::unique_ptr<char, decltype(&std::free)> p{
                abi::__cxa_demangle(name, nullptr, nullptr, nullptr),
                std::free
            };

            std::string result = p ? p.get() : name;
            if (std::is_const_v<T>) result += " const";
            if (std::is_volatile_v<T>) result += " volatile";
            if (std::is_lvalue_reference_v<T>) result += "&";
            if (std::is_rvalue_reference_v<T>) result += "&&";
            return result;
        }
    };
}

TEST_CASE("basic tests") {
    STATIC_REQUIRE(std::is_constructible_v<vd::Void>);
    STATIC_REQUIRE(std::is_copy_constructible_v<vd::Void>);
    STATIC_REQUIRE(std::is_copy_assignable_v<vd::Void>);
    STATIC_REQUIRE(std::is_move_constructible_v<vd::Void>);
    STATIC_REQUIRE(std::is_move_assignable_v<vd::Void>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<vd::Void>);

    CHECK(vd::Void() == vd::Void());
    CHECK_FALSE(vd::Void() != vd::Void());
    CHECK_FALSE(vd::Void() < vd::Void());
    CHECK(vd::Void() <= vd::Void());
    CHECK_FALSE(vd::Void() > vd::Void());
    CHECK(vd::Void() >= vd::Void());

    CHECK(type<vd::wrap_void<int>> == type<int>);
    CHECK(type<vd::wrap_void<void>> == type<vd::Void>);
    CHECK(type<vd::unwrap_void<void>> == type<void>);
    CHECK(type<vd::unwrap_void<int>> == type<int>);
    CHECK(type<vd::unwrap_void<vd::Void>> == type<void>);
}

TEST_CASE("invocation") {
    int i = 2;
    auto get = [&]{ return i; };
    auto incr = [&](int n = 1){ i += n; };
    CHECK(vd::invoke(get) == 2);
    CHECK(vd::invoke(incr, vd::Void{}) == vd::Void{});
    CHECK(vd::invoke(get, vd::Void()) == 3);
    CHECK(vd::invoke(incr, 2) == vd::Void{});
    CHECK(vd::invoke(get) == 5);

    #ifdef __cpp_concepts
    STATIC_REQUIRE_FALSE(std::invocable<decltype(get), vd::Void>);
    STATIC_REQUIRE(vd::invocable<decltype(get), vd::Void>);

    STATIC_REQUIRE(std::invocable<decltype(get)>);
    STATIC_REQUIRE(vd::invocable<decltype(get)>);
    #endif
}
