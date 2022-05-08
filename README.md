# Regular Void: The Library

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
if constexpr (std::is_void_v) {
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
    * `vd::invoke(f, vd::Void{})` is equivalent to `f()`

This library allows the above code to be handled as:

```cpp
template<class R, class... P, class Callable>
R invoke_and_log(callable_log<R(P...)>& log, Callable&& callable,
                 std::add_rvalue_reference_t<P>... args) {
  log.log_arguments(args...);
  vd::wrap_void<R> result =
             vd::invoke(std::forward<Callable>(callable),
                        std::forward<P>(args)...);
  // VD_LIFT is a macro that turns a name into a callable
  // function object. This way if `R` is `void`, this calls
  // `log.log_result()`
  vd:invoke([&] VD_LIFT(log.log_result), result);
  return result;
}
```
