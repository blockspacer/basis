#pragma once

#include <memory>
#include <variant>
#if defined(__cplusplus) && __cplusplus >= 201709L
#include <concepts>
#endif // __cplusplus

#include <base/logging.h>
#include <base/macros.h>
#include <base/location.h>
#include <basic/rvalue_cast.h>
#include <base/sequence_checker.h>
#include <base/memory/scoped_refptr.h>

namespace basis {

/// \note prefer `WeakPtr` if you do not want to affect object lifetime.
/// \note prefer `UnownedRef` to only hold non-null values (`not null`).
/// \note prefer `UnownedPtr` to only hold standard C-style pointer
//
// Either a
// * raw pointer (can be nullptr).
// * `std::unique_ptr` (owns moved `std::unique_ptr`).
// * `std::shared_ptr` (inc. ref. count of `std::shared_ptr`).
// * `scoped_refptr` from base (inc. ref. count of `scoped_refptr`).
//
// Used whenever you just want a pointer to some (polymorphic) object, and don't care
//
// Unlike std::any, preserves normal pointer behaviour.
//
// Checks lifetime of pointer using memory tool like ASAN.
//
// Checks thread-safety using `sequence_checker_`.
//
template<typename T>
struct AnyPtr {
  using pointer = T*;
  using unique_ptr = std::unique_ptr<T>;
  using shared_ptr = std::shared_ptr<T>;
  using shared_refptr = scoped_refptr<T>;

  AnyPtr() : variant_(static_cast<pointer>(nullptr))
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  AnyPtr(pointer p) noexcept : ptr_(p), variant_(p)
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    checkForLifetimeIssues();
  }

  /// \note Moves original `unique_ptr`, so becomes owner of `unique_ptr`.
  /// If you do not want to own `unique_ptr`, than use `unique_ptr::get()`.
  AnyPtr(unique_ptr&& up) noexcept : ptr_(up.get()), variant_(RVALUE_CAST(up))
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    checkForLifetimeIssues();
  }

  template<typename U>
#if defined(__cplusplus) && __cplusplus >= 201709L
  requires std::derived_from<T, U>
#endif
  AnyPtr(unique_ptr<U>&& up) noexcept : AnyPtr(static_cast<unique_ptr>(RVALUE_CAST(up)))
  {}

  /// \note Moves copy of original `shared_ptr`, so increases ref. count.
  /// If you do not want to increase ref. count of `shared_ptr`, than use `shared_ptr::get()`.
  AnyPtr(shared_ptr sp) noexcept : ptr_(sp.get()), variant_(RVALUE_CAST(sp))
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    checkForLifetimeIssues();
  }

  template<typename U>
#if defined(__cplusplus) && __cplusplus >= 201709L
  requires std::derived_from<T, U>
#endif
  AnyPtr(shared_ptr<U> sp) noexcept : AnyPtr(static_cast<shared_ptr>(RVALUE_CAST(sp)))
  {}

  /// \note Moves copy of original `shared_refptr`, so increases ref. count.
  /// If you do not want to increase ref. count of `shared_refptr`, than use `shared_refptr::get()`.
  AnyPtr(shared_refptr sp) noexcept : ptr_(sp.get()), variant_(RVALUE_CAST(sp))
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);
    checkForLifetimeIssues();
  }

  template<typename U>
#if defined(__cplusplus) && __cplusplus >= 201709L
  requires std::derived_from<T, U>
#endif
  AnyPtr(shared_refptr<U> sp) noexcept : AnyPtr(static_cast<shared_refptr>(RVALUE_CAST(sp)))
  {}

  AnyPtr(AnyPtr&&) = default;

  AnyPtr& operator=(AnyPtr&&) = default;

  inline void DetachFromSequence() {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  pointer get() const noexcept
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
        << "AnyPtrs must be checked on the same sequenced thread.";
    checkForLifetimeIssues();
    return ptr_;
  }

  T& operator*() const noexcept
  {
    return *get();
  }

  pointer operator->() const noexcept
  {
    return get();
  }

  operator bool() const noexcept
  {
    return get() != nullptr;
  }

  bool operator==(const AnyPtr&) const noexcept = default;

  bool operator==(const pointer p) const noexcept
  {
    return get() == p;
  }

  bool operator==(const unique_ptr& p) const noexcept
  {
    return get() == p;
  }

  bool operator==(const shared_ptr& p) const noexcept
  {
    return get() == p;
  }

#ifdef __cpp_impl_three_way_comparison
  std::strong_ordering operator<=>(const AnyPtr&) const noexcept = default;

  std::strong_ordering operator<=>(const pointer p) const noexcept
  {
    return get() <=> p;
  }

  std::strong_ordering operator<=>(const unique_ptr& p) const noexcept
  {
    return get() <=> p;
  }

  std::strong_ordering operator<=>(const shared_ptr& p) const noexcept
  {
    return get() <=> p;
  }

  std::strong_ordering operator<=>(const shared_refptr& p) const noexcept
  {
    return get() <=> p;
  }
#else
  bool operator!=(const AnyPtr& p) const noexcept = default;
  bool operator>=(const AnyPtr& p) const noexcept = default;
  bool operator>(const AnyPtr& p) const noexcept = default;
  bool operator<=(const AnyPtr& p) const noexcept = default;
  bool operator<(const AnyPtr& p) const noexcept = default;

  bool operator!=(const pointer p) const noexcept { return get() != p; }
  bool operator>=(const pointer p) const noexcept { return get() != p; }
  bool operator>(const pointer p) const noexcept { return get() != p; }
  bool operator<=(const pointer p) const noexcept { return get() != p; }
  bool operator<(const pointer p) const noexcept { return get() != p; }

  bool operator!=(const unique_ptr& p) const noexcept { return get() != p; }
  bool operator>=(const unique_ptr& p) const noexcept { return get() != p; }
  bool operator>(const unique_ptr& p) const noexcept { return get() != p; }
  bool operator<=(const unique_ptr& p) const noexcept { return get() != p; }
  bool operator<(const unique_ptr& p) const noexcept { return get() != p; }

  bool operator!=(const shared_ptr& p) const noexcept { return get() != p; }
  bool operator>=(const shared_ptr& p) const noexcept { return get() != p; }
  bool operator>(const shared_ptr& p) const noexcept { return get() != p; }
  bool operator<=(const shared_ptr& p) const noexcept { return get() != p; }
  bool operator<(const shared_ptr& p) const noexcept { return get() != p; }

  bool operator!=(const shared_refptr& p) const noexcept { return get() != p; }
  bool operator>=(const shared_refptr& p) const noexcept { return get() != p; }
  bool operator>(const shared_refptr& p) const noexcept { return get() != p; }
  bool operator<=(const shared_refptr& p) const noexcept { return get() != p; }
  bool operator<(const shared_refptr& p) const noexcept { return get() != p; }
#endif // __cpp_impl_three_way_comparison

 private:
  // check that object is alive, use memory tool like ASAN
  /// \note ignores nullptr
  inline void checkForLifetimeIssues() const
  {
    // Works with `-fsanitize=address,undefined`
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
    if (ptr_ != nullptr)
      reinterpret_cast<const volatile uint8_t*>(ptr_)[0];
#endif
  }

private:
  /// Cached, so we dont have to switch on the variant type for each access
  pointer ptr_;
  std::variant<pointer, unique_ptr, shared_ptr, shared_refptr> variant_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(AnyPtr);
};

} // namespace basis
