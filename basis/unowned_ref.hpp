#pragma once

#include <functional>
#include <type_traits>

#include <base/logging.h>
#include <base/macros.h>
#include <base/threading/thread_collision_warner.h>

#include <basis/concept/is_reference_wrapper.hpp>

namespace basis {

/// \note Prefer `UnownedRef` to `gsl::not_null`, `std::reference_wrapper` etc.
// UnownedRef is similar to std::reference_wrapper
//
// 1. It documents the nature of the reference with no need to add a comment
//    explaining that is it // Not owned.
//
// 2. Supports memory tools like ASAN
//
// 3. Can construct from std::ref and std::cref
//    (like std::reference_wrapper)
//
// 4. Assignment can change internal pointer,
//    but only if UnownedRef was not initialized
//    (NOT like std::reference_wrapper)
//
// 5. Because |UnownedRef| expected to be NOT modified after construction,
//    it is more thread-safe than |UnownedPtr|

template <class Type>
class UnownedRef
{
public:
  UnownedRef() = default;
  UnownedRef(const UnownedRef& that)
    : UnownedRef(that.Ref())
  {}

  NOT_THREAD_SAFE_FUNCTION()
  UnownedRef(
    UnownedRef&& other)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    DCHECK(!pObj_)
      << "UnownedRef construction can change internal pointer,"
      << " but only if UnownedRef was not initialized."
      << " You may want to create new UnownedRef.";

    checkForLifetimeIssues();
    if (*this != other) {
      if(pObj_ != other.Get())
      {
        pObj_ = other.Get();
      }
      // it is reference, so can not be nullptr
      DCHECK(pObj_);
    }
  }

  NOT_THREAD_SAFE_FUNCTION()
  UnownedRef& operator=(
    const UnownedRef& that)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    DCHECK(!pObj_)
      << "UnownedRef assignment can change internal pointer,"
      << " but only if UnownedRef was not initialized."
      << " You may want to create new UnownedRef.";

    checkForLifetimeIssues();
    if (*this != that) {
      if(pObj_ != that.Get()) {
        pObj_ = that.Get();
      }
      DCHECK(pObj_);
    }
    return *this;
  }

  template <
    typename U
    , std::enable_if_t<
      !is_reference_wrapper<std::decay_t<U> >::value
      , void
      >
    >
  explicit UnownedRef(
    UNOWNED_LIFETIME(const U& pObj))
    : COPIED(pObj_(&pObj))
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    DCHECK(pObj_);
  }

  template <
    typename U
    >
  UnownedRef(
    UNOWNED_LIFETIME(const std::reference_wrapper<U>& pObj))
    : COPIED(pObj_(&pObj.get()))
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    DCHECK(pObj_);
  }

  ~UnownedRef()
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    // It is reference, so can not be `nullptr`.
    // So `pObj_` can be `nullptr` only if `UnownedRef`
    // is not constructed.
    // We are in destructor,
    // so `UnownedRef` MUST be constructed.
    DCHECK(pObj_);

    checkForLifetimeIssues();
  }

  // Use to change stored |UnownedRef| only in specific
  // places like move assignment operator.
  // Avoid `reset()` if you can
  // because |UnownedRef| expected
  // to be NOT changed after construction
  // (but some patterns like object pool require
  // possibility of modification to avoid allocations).
  NOT_THREAD_SAFE_FUNCTION()
  void reset(
    UNOWNED_LIFETIME(Type*) that)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    // it is reference, so can not be nullptr
    DCHECK(that);

    checkForLifetimeIssues();
    if(pObj_ != that) {
      pObj_ = that;
    }
  }

  bool operator==(const UnownedRef& that) const
  {
    return Get() == that.Get();
  }

  bool operator!=(const UnownedRef& that) const
  {
    return !(*this == that);
  }

  bool operator<(const UnownedRef& that) const
  {
    return std::less<Type*>()(Get(), that.Get());
  }

  template <typename U>
  bool operator==(const U& that) const
  {
    return Get() == &that;
  }

  template <typename U>
  bool operator!=(const U& that) const
  {
    return !(*this == &that);
  }

  Type& Ref() const
  {
    DCHECK(pObj_);
    return *pObj_;
  }

  /// \note Do not do stupid things like
  /// `delete unownedRef.Get();` // WRONG
  Type* Get() const
  {
    return pObj_;
  }

  Type& operator*() const
  {
    DCHECK(pObj_);
    return *pObj_;
  }

  Type* operator->() const
  {
    DCHECK(pObj_);
    return pObj_;
  }

private:
  // check that object is alive, use memory tool like ASAN
  inline void checkForLifetimeIssues() const
  {
    // Works with `-fsanitize=address,undefined`
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
    if (pObj_)
      reinterpret_cast<const volatile uint8_t*>(pObj_)[0];
#endif
  }

  /// \note Thread collision warner used only for modification operations
  /// because you may want to use unchangable storage
  /// that can be read from multiple threads safely.
  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  // It is reference, so can not be `nullptr`.
  // So `pObj_` can be `nullptr` only if `UnownedRef`
  // is not constructed.
  Type* pObj_
    = nullptr;
};

template <typename Type, typename U>
inline bool operator==(const U& lhs, const UnownedRef<Type>& rhs)
{
  return rhs == lhs;
}

template <typename Type, typename U>
inline bool operator!=(const U& lhs, const UnownedRef<Type>& rhs)
{
  return rhs != lhs;
}

}  // namespace basis
