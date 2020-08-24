#pragma once

#include <base/macros.h>
#include <base/logging.h>
#include <base/sequence_checker.h>

#include <basis/dependent_false.hpp>

#include <type_traits>

namespace util {

// Use it to make sure that you `copy-only-once` or `move`.
// It is good practice to document
// `copy-only-once` operation via |MoveOnly| for large data types.
/// \note |MoveOnly| is movable but NOT copiable
/// to make sure that you copy large data type ONLY ONCE!
template <
  class T
  , typename = void
  >
class MoveOnly
{
  static_assert(
    typename_false<T>::value
    , "unable to find MoveOnly implementation");
};

template <
  class T
  >
class MoveOnly<
  T
  , std::enable_if_t<
  !std::is_const<T> {}
// you may want to use |UnownedPtr|
// if you want to wrap pointer
&& !std::is_pointer<T>{}
, void
>
>
{
private:
  // Made private bacause it makes
  // `move` operation implicit.
  // Use |moveFrom| instead.
  explicit MoveOnly(T&& scoper)
    : is_valid_(true)
    , scoper_(std::move(scoper))
  {
  }

public:
  // We want to explicitly document that `copy` operation will happen
  static MoveOnly copyFrom(COPIED(const T & scoper))
  {
    T tmp = scoper;
    return MoveOnly(std::move(tmp));
  }

  // We want to explicitly document that `move` operation will happen
  static MoveOnly moveFrom(T&& scoper)
  {
    return MoveOnly(std::move(scoper));
  }

  /// \note |MoveOnly| must be movable but NOT copiable
  /// to make sure that you copy large data type ONLY ONCE!
  MoveOnly(MoveOnly&& other)
    : is_valid_(other.is_valid_)
    , scoper_(std::move(other.scoper_))
  {
  }

  MUST_USE_RETURN_VALUE
  T&& Take() const
  {
    // call |Take()| only once and only from one thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    CHECK(is_valid_);
    is_valid_ = false;
    return std::move(scoper_);
  }

  MoveOnly(MoveOnly const&) = delete;

  MoveOnly& operator=(MoveOnly const&) = delete;

private:
  // is_valid_ is distinct from NULL
  mutable bool is_valid_;

  mutable T scoper_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_NEW_OPERATOR(MoveOnly);
};

// version of |MoveOnly| for `const T` data types
template <
  class T
  >
class MoveOnly<
  const T
  , std::enable_if_t<
  // you may want to use |UnownedPtr|
  // if you want to wrap pointer
  !std::is_pointer<T> {}
, void
>
>
{
private:
  // Made private bacause it makes
  // `move` operation implicit.
  // Use |moveFrom| instead.
  explicit MoveOnly(T&& scoper)
    : is_valid_(true)
    , scoper_(std::move(scoper))
  {
  }

public:
  // We want to explicitly document that `copy` operation will happen
  static MoveOnly copyFrom(COPIED(const T & scoper))
  {
    T tmp = scoper;
    return MoveOnly(std::move(tmp));
  }

  // We want to explicitly document that `move` operation will happen
  static MoveOnly moveFrom(T&& scoper)
  {
    return MoveOnly(std::move(scoper));
  }

  /// \note |MoveOnly| must be movable but NOT copiable
  /// to make sure that you copy large data type ONLY ONCE!
  MoveOnly(MoveOnly&& other)
    : is_valid_(other.is_valid_)
    , scoper_(std::move(other.scoper_))
  {
  }

  MUST_USE_RETURN_VALUE
  const T&& TakeConst() const
  {
    // call |Take()| only once and only from one thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    CHECK(is_valid_);
    is_valid_ = false;
    return std::move(scoper_);
  }

  MoveOnly(MoveOnly const&) = delete;

  MoveOnly& operator=(MoveOnly const&) = delete;

private:
  // is_valid_ is distinct from NULL
  mutable bool is_valid_;

  mutable T scoper_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_NEW_OPERATOR(MoveOnly);
};

} // namespace util
