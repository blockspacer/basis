#pragma once

#include "basis/bind/verify_nothing.hpp"

#include <boost/algorithm/string.hpp>

#include <base/macros.h>
#include <base/sequence_checker.h>
#include <base/callback.h>
#include <base/optional.h>
#include <base/location.h>
#include <base/rvalue_cast.h>
#include <base/bind_helpers.h>
#include <base/strings/string_piece.h>
#include <base/threading/thread_collision_warner.h>

#include <basis/core/bitmask.hpp>

#include <functional>
#include <map>
#include <string>

/// \note Prefer custom annotations with checks (see `checks_and_guard_annotations.hpp`)
/// to `CheckedOptional`
//
namespace basis {

enum class CheckedOptionalPermissions
{
  None
      = 0
  // If `VerifyNothing::hasReadPermission` is true,
  // when you can read stored in `VerifyNothing` value
  // depending on `CheckedOptionalPolicy`.
  // For example, if `VerifyNothing::hasReadPermission` is false
  // and `CheckedOptionalPolicy` is not `Skip`, then
  // checks may be perfomed during call to
  // `VerifyNothing::operator->()`
  , Readable
      = 1 << 1
  // If `VerifyNothing::hasModifyPermission` is true,
  // when you can change stored in `VerifyNothing` value
  // depending on `CheckedOptionalPolicy`.
  // For example, if `VerifyNothing::hasModifyPermission` is false
  // and `CheckedOptionalPolicy` is not `Skip`, then
  // checks may be perfomed during call to
  // `VerifyNothing::emplace`
  , Modifiable
      = 1 << 2
  , All
      = CheckedOptionalPermissions::Readable
        | CheckedOptionalPermissions::Modifiable
};
ALLOW_BITMASK_OPERATORS(CheckedOptionalPermissions)

enum class CheckedOptionalPolicy {
  // Will call `verifier_callback_.Run()` in any builds (including release),
  // so take care of performance
  Always
  // Will call `verifier_callback_.Run()` only in debug builds,
  // prefer for performance reasons
  , DebugOnly
  // Can be used to implement custom verification logic
  , Skip
};

// Used to verify that each access to underlying type
// is protected by conditional function.
//
/// \note All checks performed on storage of `Type` object
/// (i.e. controls modification of `base::Optional`),
/// not on API of `Type` object.
/// You must also control thread-safety and permissions
/// in API of `Type` object.
//
// MOTIVATION
//
// Similar to |base::Optional|, but with extra checks on each use
// i.e. calls `VerifierCb` that must return true if check passed.
//
// If you want to make object immutable,
// than you can change `CheckedOptionalPermissions`.
//
// USAGE
//
//  // The io_context is required for all I/O
//  ::basis::CheckedOptional<
//    boost::asio::io_context
//    , ::basis::CheckedOptionalPolicy::DebugOnly
//  > ioc_{
//      // it safe to read value from any thread
//      // because its storage expected to be not modified
//      ::basis::VerifyNothing::Repeatedly()
//      , ::basis::CheckedOptionalPermissions::Readable
//      , ::base::in_place};
//
//  ::basis::CheckedOptional<
//    StateMachineType
//  > sm_(
//    BIND_UNRETAINED_RUN_ON_STRAND_CHECK(&acceptorStrand_) // see CheckedOptional constructor
//    // "disallow `emplace` for thread-safety reasons"
//    , ::basis::CheckedOptionalPermissions::Readable // see CheckedOptional constructor
//    , ::base::in_place // see ::base::Optional constructor
//    , UNINITIALIZED // see StateMachineType constructor
//    , FillStateTransitionTable()) // see StateMachineType constructor
//
//  sm_.forceValidToModify(FROM_HERE
//    , "allow `emplace`"
//  );
//
//  sm_.forceNotValidToModify(FROM_HERE
//    , "disallow `emplace` for thread-safety reasons"
//  );
template<
  typename Type
  , CheckedOptionalPolicy VerifyPolicyType
>
class CheckedOptional
{
public:
  // May be called on each member function call
  // depending on `CheckedOptionalPolicy`.
  // Usually it is used for thread-safety checks.
  using VerifierCb
    = ::base::RepeatingCallback<bool()>;

public:
  /// \note if you want to initialize `value_`,
  /// than you can pass `base::in_place` as second argument
  explicit CheckedOptional(
    VerifierCb&& verifierCb
    , const ::basis::CheckedOptionalPermissions& permissions
        = ::basis::CheckedOptionalPermissions::All)
    : verifier_callback_(verifierCb)
    , CheckedOptionalPermissions(permissions)
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);

    DCHECK(!value_);
  }

  /// \note you may want to pass `base::in_place` as second argument
  template <class... Args>
  CheckedOptional(
    VerifierCb&& verifierCb
    , const ::basis::CheckedOptionalPermissions& permissions
    , Args&&... args)
    : verifier_callback_(verifierCb)
    , CheckedOptionalPermissions(permissions)
    , value_(std::forward<Args>(args)...)
  {
    DETACH_FROM_SEQUENCE(sequence_checker_);

    DCHECK(value_);
  }

  ~CheckedOptional()
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  bool runVerifierCallback() const NO_EXCEPTION
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Skip)
    {
      NOTREACHED();
    }

    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly
      && !DCHECK_IS_ON())
    {
      NOTREACHED();
    }

    DCHECK(verifier_callback_);
    return verifier_callback_.Run();
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  bool hasReadPermission() const NO_EXCEPTION
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Skip)
    {
      NOTREACHED();
    }

    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly
      && !DCHECK_IS_ON())
    {
      NOTREACHED();
    }

    return ::basis::hasBit(CheckedOptionalPermissions
          , ::basis::CheckedOptionalPermissions::Readable);
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  bool hasModifyPermission() const NO_EXCEPTION
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Skip)
    {
      NOTREACHED();
    }

    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly
      && !DCHECK_IS_ON())
    {
      NOTREACHED();
    }

    return
      ::basis::hasBit(CheckedOptionalPermissions
          , ::basis::CheckedOptionalPermissions::Modifiable);
  }

  /// \note performs automatic checks only in debug mode,
  /// in other modes you must call `runVerifierCallback()` manually
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ::base::Optional<Type>& optional()
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      CHECK(hasReadPermission())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();
    }

    return optional_unsafe(FROM_HERE, "");
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ::base::Optional<Type>& optional() const
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      CHECK(hasReadPermission())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();
    }

    return optional_unsafe(FROM_HERE, "");
  }

  // Similar to |optional|, but without thread-safety checks
  // Usually you want to use *_unsafe in destructors
  // (when data no longer used between threads)
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  ::base::Optional<Type>& optional_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    , ::base::OnceClosure&& check_unsafe_allowed = ::base::DoNothing::Once())
  {
    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();
    return value_;
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const ::base::Optional<Type>& optional_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    , ::base::OnceClosure&& check_unsafe_allowed = ::base::DoNothing::Once()) const
  {
    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();
    return value_;
  }

  /// \note performs automatic checks only in debug mode,
  /// in other modes you must call `runVerifierCallback()` manually
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  Type& value()
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      CHECK(hasReadPermission())
        << FROM_HERE.ToString();
      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();
      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_unsafe(FROM_HERE, "");
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const Type& value() const
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      CHECK(hasReadPermission())
        << FROM_HERE.ToString();
      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();
      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();
      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_unsafe(FROM_HERE, "");
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  Type& value_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    , ::base::OnceClosure&& check_unsafe_allowed = ::base::DoNothing::Once())
  {
    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();

    return value_.value();
  }

  // Similar to |value|, but without thread-safety checks
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const Type& value_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    , ::base::OnceClosure&& check_unsafe_allowed = ::base::DoNothing::Once()) const
  {
    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();
    return value_.value();
  }

  constexpr const Type& operator*() const
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      CHECK(hasReadPermission())
        << FROM_HERE.ToString();

      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();

      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_.operator*();
  }

  constexpr Type& operator*()
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      CHECK(hasReadPermission())
        << FROM_HERE.ToString();

      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();

      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_.operator*();
  }

  constexpr const Type* operator->() const
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      CHECK(hasReadPermission())
        << FROM_HERE.ToString();

      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();

      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_.operator->();
  }

  constexpr Type* operator->()
  {
    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      CHECK(hasReadPermission())
        << FROM_HERE.ToString();

      CHECK(value_.has_value())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      DCHECK(hasReadPermission())
        << FROM_HERE.ToString();

      DCHECK(value_.has_value())
        << FROM_HERE.ToString();
    }

    return value_.operator->();
  }

  template<
    class... Args
  >
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  Type& emplace(
    const ::base::Location& from_here
    , Args&&... args)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << from_here.ToString();

      CHECK(hasModifyPermission())
        << from_here.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << from_here.ToString();

      DCHECK(hasModifyPermission())
        << from_here.ToString();
    }

    return value_.emplace(std::forward<Args>(args)...);
  }

  // Similar to |emplace|, but without thread-safety checks
  template<
    class... Args
  >
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  Type& emplace_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    // usually you want to pass `= ::base::DoNothing::Once()` here
    , ::base::OnceClosure&& check_unsafe_allowed
    , Args&&... args)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();

    return value_.emplace(std::forward<Args>(args)...);
  }

  void reset(const ::base::Location& from_here)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    if constexpr (VerifyPolicyType == CheckedOptionalPolicy::Always)
    {
      CHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      CHECK(hasModifyPermission())
        << FROM_HERE.ToString();
    }
    else if constexpr (VerifyPolicyType == CheckedOptionalPolicy::DebugOnly && DCHECK_IS_ON())
    {
      DCHECK(runVerifierCallback())
        << FROM_HERE.ToString();

      DCHECK(hasModifyPermission())
        << FROM_HERE.ToString();
    }

    value_.reset();
  }

  void reset_unsafe(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_using_unsafe
    // usually you want to pass `= ::base::DoNothing::Once()` here
    , ::base::OnceClosure&& check_unsafe_allowed)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_using_unsafe);
    ::base::rvalue_cast(check_unsafe_allowed).Run();

    value_.reset();
  }

  void forceNotValidToRead(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_make_invalid)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DCHECK(hasReadPermission());

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_make_invalid);

    ::basis::removeBit(CheckedOptionalPermissions
      , ::basis::CheckedOptionalPermissions::Readable);
  }

  void forceNotValidToModify(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_make_invalid)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DCHECK(hasModifyPermission());

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_make_invalid);

    ::basis::removeBit(CheckedOptionalPermissions
      , ::basis::CheckedOptionalPermissions::Modifiable);
  }

  void forceValidToRead(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_make_valid)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DCHECK(!hasReadPermission());

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_make_valid);

    ::basis::addBit(CheckedOptionalPermissions
      , ::basis::CheckedOptionalPermissions::Readable);
  }

  void forceValidToModify(
    const ::base::Location& from_here
    , ::base::StringPiece reason_why_make_valid)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DCHECK(!hasModifyPermission());

    UNREFERENCED_PARAMETER(from_here);
    UNREFERENCED_PARAMETER(reason_why_make_valid);

    ::basis::addBit(CheckedOptionalPermissions
      , ::basis::CheckedOptionalPermissions::Modifiable);
  }

  bool operator==(const CheckedOptional& that) const
  {
    return value() == that.value();
  }

  bool operator!=(const CheckedOptional& that) const
  {
    return !(*this == that);
  }

  bool operator<(const CheckedOptional& that) const
  {
    return std::less<Type*>()(value(), that.value());
  }

  template <typename U>
  bool operator==(const U& that) const
  {
    return value() == &that;
  }

  template <typename U>
  bool operator!=(const U& that) const
  {
    return !(*this == &that);
  }

private:
  VerifierCb verifier_callback_;

  // MOTIVATION
  //
  // We already have custom validation function `verifier_callback_`,
  // but it is too common to make object not valid using
  // `std::move` (just mark invalid after move)
  // or to force object to initialize only once
  // (just mark invalid after initialization to prohibit `emplace()`)
  ::basis::CheckedOptionalPermissions CheckedOptionalPermissions{
    ::basis::CheckedOptionalPermissions::All};

  ::base::Optional<Type> value_;

  /// \note Thread collision warner used only for modification operations
  /// because you may want to use unchangable storage
  /// that can be read from multiple threads safely.
  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  // check sequence on which class was constructed/destructed/configured
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(CheckedOptional);
};

template <typename U, class... Args>
inline bool operator==(const U& lhs, const CheckedOptional<Args...>& rhs)
{
  return rhs == lhs;
}

template <typename U, class... Args>
inline bool operator!=(const U& lhs, const CheckedOptional<Args...>& rhs)
{
  return rhs != lhs;
}

} // namespace basis
