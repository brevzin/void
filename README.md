# Regular Void: The Library

![Tests](https://github.com/brevzin/void/actions/workflows/main.yml/badge.svg)

[P0146](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0146r1.html) describes the issue at hand pretty well: in generic code, we often want to deal with functions that can return any type - and we don't actually care what that type is. Example from the paper:

```cpp
// Invoke a Callable, logging its arguments and return value.
// Requires an exact match of Callable&&'s pseudo function type and R(P...).
template<class R, class... P, class Callable>
R invoke_and_log(callable_log<R(P...)>& log, Callable&& callable,
                 std::add_rvalue_reference_t<P>... args) {
  log.log_arguments(args...);
  R result = std::invoke(std::forward<Callable>(callable),
                         std::forward<P>(args)...);
  log.log_result(result);
  return result;
}
```

This is correct for all `R` except `void`. For `void`, specifically, we need dedicated handling:

```cpp
if constexpr (std::is_void_v<R>) {
    std::invoke(std::forward<Callable>(callable),
                std::forward<P>(args)...);
    log.log_result();
} else {
    R result = std::invoke(std::forward<Callable>(callable),
                           std::forward<P>(args)...);
    log.log_result(result);
    return result;
}
```

This is tedious and error-prone. While P0146 proposed a language solution to this problem (letting the original code just work with `R=void`), the paper also pointed out that this could be helped with a library-based solution.

This is such a (C++17) library-based solution.

It features the following:

* a type, `vd::Void`, that is a regular unit type.
* metafunctions `vd::wrap_void` and `vd::unwrap_void` that convert `void` to `vd::Void` and back.
* a function `vd::invoke` that is similar to `std::invoke` except that:

    * it returns `vd::Void` instead of `void`, where appropriate, and
    * `vd::invoke(f, vd::Void{})` is equivalent to `vd::invoke(f)` (regardless of whether `f` is invocable with `vd::Void`). See [rationale](#design-rationale).

* a metafunction `vd::void_result_t` that is to `vd::invoke` what `std::invoke_result_t` is to `std::invoke`, except that it still gives you `void` (instead of `Void`). See [rationale](#design-rationale).

* (on C++20) a concept `vd::invocable` that is to `vd::invoke` what `std::invocable` is to `std::invoke`

This library allows the above code to be handled as:

```cpp
template<class R, class... P, class Callable>
vd::wrap_void<R> invoke_and_log(
                 callable_log<R(P...)>& log, Callable&& callable,
                 std::add_rvalue_reference_t<P>... args) {
  log.log_arguments(args...);
  vd::wrap_void<R> result = vd::invoke(VD_FWD(callable), VD_FWD(args)...);
  // either pass result in directly, if passing Void is acceptable
  log.log_result(result);
  // Or use vd::invoke again to be able to call log.log_result()
  // in the void case.
  // VD_LIFT is a macro that turns a name into a function object.
  vd:invoke([&] VD_LIFT(log.log_result), result);
  return result;
}
```

## Design Rationale

There is basically only one interesting design choice in this library (the rest
kind just falls out of wanting to solve the problem) and that is having
`vd::invoke(f, vd::Void{})` fallback to trying `f()` if `f(vd::Void{})` is not a valid expression.

The reason for this (along with the corresponding unwrap in `vd::void_result_t`)
is that it ends up being more useful for common use-cases. Such as:

```cpp
template <class T>
class Optional {
    union { vd::wrap_void<T> value_; };
    bool engaged_;

public:
    template <class F>
    auto map(F f) const -> Optional<vd::void_result_t<F&, vd::wrap_void<T> const&>>
    {
        if (engaged_) {
            return vd::invoke(f, value_);
        } else {
            return {};
        }
    }
};
```

For `Optional<void>` (a seemingly pointless, yet nevertheless useful
specialization), this allows `map` to take a nullary function. And for
`Optional<T>`, if `F` returns `void`, we get back `Optional<void>`. Everything
works out quite nicely.
