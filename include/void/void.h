#ifndef VD_VOID_H
#define VD_VOID_H

#include <type_traits>
#include <functional>
#include <version>

#if defined(__cpp_lib_three_way_comparison)
#  include <compare>
#endif

#define VD_FWD(x) static_cast<decltype(x)&&>(x)

// [] VD_LIFT(name) gives you an object f such that f(args...)
// is equivalent to name(args...)
// This macro must follow a lambda-introducer.
#define VD_LIFT(name)                            \
    (auto&&... args)                             \
    noexcept(noexcept(name(VD_FWD(args)...)))    \
    -> decltype(name(VD_FWD(args)...))           \
    { return name(VD_FWD(args)...); }

#if defined(__cpp_lib_constexpr_functional) && __cpp_lib_constexpr_functional >= 201907
#  define VD_CONSTEXPR constexpr
#else
#  define VD_CONSTEXPR
#endif

namespace vd {

struct Void {
    Void() = default;
    Void(Void const&) = default;
    Void(Void&&) = default;
    Void& operator=(Void const&) = default;
    Void& operator=(Void&&) = default;

    template <class Arg, class... Args
        #if !defined(__cpp_concepts)
        , std::enable_if_t<
            !std::is_base_of_v<Void, std::remove_reference_t<Arg>>,
            int> = 0
        #endif
        >
        #if defined(__cpp_concepts)
        requires (!std::is_base_of_v<Void, std::remove_reference_t<Arg>>)
        #endif
    constexpr explicit Void(Arg&&, Args&&...) { }

    friend constexpr auto operator==(Void, Void) -> bool {
        return true;
    }
    #if defined(__cpp_lib_three_way_comparison)
    friend constexpr auto operator<=>(Void, Void) -> std::strong_ordering {
        return std::strong_ordering::equal;
    }
    #else
    friend constexpr auto operator!=(Void, Void) -> bool {
        return false;
    }
    friend constexpr auto operator< (Void, Void) -> bool {
        return false;
    }
    friend constexpr auto operator<=(Void, Void) -> bool {
        return true;
    }
    friend constexpr auto operator> (Void, Void) -> bool {
        return false;
    }
    friend constexpr auto operator>=(Void, Void) -> bool {
        return true;
    }
    #endif
};

namespace detail {
    template <bool> struct meta;
    template <> struct meta<true> {
        template <class T, class U> using if_c = T;
    };
    template <> struct meta<false> {
        template <class T, class U> using if_c = U;
    };

    template <bool B, class T, class U>
    using if_c = typename meta<B>::template if_c<T, U>;
}

// Void if T is cv void, else T
template <class T>
using wrap_void = detail::if_c<std::is_void_v<T>, Void, T>;

// void if T is Void cvref, else T
template <class T>
using unwrap_void = detail::if_c<
    std::is_same_v<
        #ifdef __cpp_lib_remove_cvref
        std::remove_cvref_t<T>,
        #else
        std::remove_cv_t<std::remove_reference_t<T>>,
        #endif
        Void
        >, void, T>;

namespace detail {
    struct invoke_t {
        template <class F, class... Args,
                class R = std::invoke_result_t<F, Args...>>
        VD_CONSTEXPR auto operator()(F&& f, Args&&... args) const -> wrap_void<R> {
            if constexpr (not std::is_void_v<R>) {
                return std::invoke(VD_FWD(f), VD_FWD(args)...);
            } else {
                std::invoke(VD_FWD(f), VD_FWD(args)...);
                return Void{};
            }
        }

        // special-case invoke(f, Void()). if f(Void()) isn't a viable call
        // then call f()
        template <class F, class Arg, class R = std::invoke_result_t<F>
            #ifndef __cpp_concepts
            , std::enable_if_t<
                std::is_same_v<std::decay_t<Arg>, Void>
                && !std::is_invocable_v<F, Arg>
                , int> = 0
            #endif
            >
            #ifdef __cpp_concepts
            requires std::same_as<std::decay_t<Arg>, Void>
                  && (!std::invocable<F, Arg>)
            #endif
        VD_CONSTEXPR auto operator()(F&& f, Arg&& ) const -> wrap_void<R> {
            if constexpr (not std::is_void_v<R>) {
                return VD_FWD(f)();
            } else {
                VD_FWD(f)();
                return Void{};
            }
        }
    };
}

inline constexpr detail::invoke_t invoke{};

// void-specific version of invoke_result_t that still returns void, not Void
template <class F, class... Args>
using void_result_t = unwrap_void<
    decltype(vd::invoke(std::declval<F>(), std::declval<Args>()...))
    >;

#ifdef __cpp_concepts
namespace detail {
    template <class T>
    using Producer = T(*)();
}

template <class F, class... Args>
concept invocable = requires(detail::Producer<F> f, detail::Producer<Args>... args) {
    vd::invoke(f(), args()...);
};
#endif

}

#endif
