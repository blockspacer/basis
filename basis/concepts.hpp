#pragma once

// USAGE
//
// template <typename Func>
// CONCEPT( requires std::is_nothrow_move_constructible_v<Func> )
// auto
// moveFunc(Func func) noexcept {
//   static_assert(std::is_nothrow_move_constructible_v<Func>);
//   return std::move(func);
// }
#if defined(__cpp_concepts) && __cpp_concepts >= 201907

#define CONCEPT(x...) x
#define NO_CONCEPT(x...)

#else

#define CONCEPT(x...)
#define NO_CONCEPT(x...) x

#endif // __cpp_concepts
