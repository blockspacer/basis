#pragma once

#include <base/logging.h>
#include <base/macros.h>
#include <base/threading/thread_collision_warner.h>

#include <cstddef>
#include <functional>

/// \note Prefer `UnownedRef` if stored pointer
/// always set (can not be nullptr)
/// i.e. `UnownedPtr` should be almost never used.
// UnownedPtr is a smart pointer class that behaves very much like a
// standard C-style pointer. The advantages of using it over raw
// pointers are:
//
// 1. It documents the nature of the pointer with no need to add a comment
//    explaining that is it // Not owned. Additionally, an attempt to delete
//    an unowned ptr will fail to compile rather than silently succeeding,
//    since it is a class and not a raw pointer.
//
// 2. When built for a memory tool like ASAN, the class provides a destructor
//    which checks that the object being pointed to is still alive.
//
// Hence, when using UnownedPtr, no dangling pointers are ever permitted,
// even if they are not de-referenced after becoming dangling. The style of
// programming required is that the lifetime an object containing an
// UnownedPtr must be strictly less than the object to which it points.
//
// The same checks are also performed at assignment time to prove that the
// old value was not a dangling pointer, either.
//
// The array indexing operation [] is not supported on an unowned ptr,
// because an unowned ptr expresses a one to one relationship with some
// other heap object.

namespace basis {

template <class Type>
class UnownedPtr
{
public:
  UnownedPtr() = default;
  UnownedPtr(const UnownedPtr& that)
    : UnownedPtr(that.Get())
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);
  }

  template <typename U>
  explicit UnownedPtr(
    UNOWNED_LIFETIME(U* pObj))
    : COPIED(pObj_(pObj))
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    DCHECK(pObj_);
  }

  // Deliberately implicit to allow returning nullptrs.
  // NOLINTNEXTLINE(runtime/explicit)
  UnownedPtr(std::nullptr_t ptr)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    ignore_result(ptr);
  }

  ~UnownedPtr()
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
  }

  NOT_THREAD_SAFE_FUNCTION()
  UnownedPtr(
    UnownedPtr&& other)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
    if (*this != other) {
      if(pObj_ != other.Get())
      {
        pObj_ = other.Get();
      }
      DCHECK(pObj_);
    }
  }

  NOT_THREAD_SAFE_FUNCTION()
  UnownedPtr& operator=(
    UnownedPtr&& other)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
    if (*this != other) {
      if(pObj_ != other.Get())
      {
        pObj_ = other.Get();
      }
      DCHECK(pObj_);
    }
    return *this;
  }

  NOT_THREAD_SAFE_FUNCTION()
  UnownedPtr& operator=(
    UNOWNED_LIFETIME(Type*) that)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
    if(pObj_ != that)
    {
      pObj_ = that;
    }
    return *this;
  }

  NOT_THREAD_SAFE_FUNCTION()
  UnownedPtr& operator=(
    const UnownedPtr& that)
  {
    DFAKE_SCOPED_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
    if (*this != that) {
      if(pObj_ != that.Get())
      {
        pObj_ = that.Get();
      }
      DCHECK(pObj_);
    }
    return *this;
  }

  NOT_THREAD_SAFE_FUNCTION()
  bool operator==(
    const UnownedPtr& that) const
  {
    return Get() == that.Get();
  }

  NOT_THREAD_SAFE_FUNCTION()
  bool operator!=(
    const UnownedPtr& that) const
  {
    return !(*this == that);
  }

  NOT_THREAD_SAFE_FUNCTION()
  bool operator<(
    const UnownedPtr& that) const
  {
    return std::less<Type*>()(Get(), that.Get());
  }

  NOT_THREAD_SAFE_FUNCTION()
  template <typename U>
  bool operator==(
    const U* that) const
  {
    return Get() == that;
  }

  NOT_THREAD_SAFE_FUNCTION()
  template <typename U>
  bool operator!=(
    const U* that) const
  {
    return !(*this == that);
  }

  /// \note Does not call `checkForLifetimeIssues`,
  /// so call it manually.
  /// \note Do not do stupid things like
  /// `delete unownedPtr.Get();` // WRONG
  NOT_THREAD_SAFE_FUNCTION()
  Type* Get() const
  {
    return pObj_;
  }

  /// \note it does not destruct |pObj_|,
  /// it just sets |pObj_| to nullptr without memory deallocation
  /// Use `Release()` when you want to delete |pObj_|
  NOT_THREAD_SAFE_FUNCTION()
  Type* Release()
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    checkForLifetimeIssues();
    Type* pTemp = nullptr;
    std::swap(pTemp, pObj_);
    DCHECK(!pObj_); // must become prev. |pTemp| i.e. nullptr
    return pTemp;
  }

  NOT_THREAD_SAFE_FUNCTION()
  explicit operator bool() const
  {
    return !!pObj_;
  }

  // implicit conversion
  NOT_THREAD_SAFE_FUNCTION()
  operator Type&() NO_EXCEPTION
  {
    DCHECK(pObj_);
    return *pObj_;
  }

  NOT_THREAD_SAFE_FUNCTION()
  Type& operator*() const
  {
    DCHECK(pObj_);
    return *pObj_;
  }

  NOT_THREAD_SAFE_FUNCTION()
  Type* operator->() const
  {
    DCHECK(pObj_);
    return pObj_;
  }

  // check that object is alive, use memory tool like ASAN
  inline void checkForLifetimeIssues() const
  {
    // Works with `-fsanitize=address,undefined`
#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
    if (pObj_)
      reinterpret_cast<const volatile uint8_t*>(pObj_)[0];
#endif
  }

private:
  /// \note Thread collision warner used only for modification operations
  /// because you may want to use unchangable storage
  /// that can be read from multiple threads safely.
  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  Type* pObj_ = nullptr;
};

template <typename Type, typename U>
inline bool operator==(const U* lhs, const UnownedPtr<Type>& rhs)
{
  return rhs == lhs;
}

template <typename Type, typename U>
inline bool operator!=(const U* lhs, const UnownedPtr<Type>& rhs)
{
  return rhs != lhs;
}

}  // namespace basis
