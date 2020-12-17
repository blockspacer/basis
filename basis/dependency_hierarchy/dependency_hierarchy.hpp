#pragma once

#include <base/logging.h>
#include <base/macros.h>
#include <base/callback.h>
#include <base/rvalue_cast.h>
#include <base/sequence_checker.h>
#include <base/containers/flat_map.h>
#include <base/threading/thread_collision_warner.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <base/observer_list_threadsafe.h>
#include <base/recursion_checker.h>

#include <basis/status/status_macros.hpp>

#include <set>
#include <string>

// MOTIVATION
//
// Assumed to be used with large, long-lived set of functionality
// that may be enabled or disabled at runtime that can depend
// on other long-lived functionality (for example: plugins,
// hierarchy of ECS systems, global static components, etc.).
// Use to enable or disable functionality based on dependency order
// (plugin initialization or termination may depend on other plugins).
//
// We use `scoped_refptr` because same dependencies may be shared between
// different sets of long-lived functionality.
//
// PERFORMANCE
//
// Large, long-lived set of functionality is not expected to be created often,
// so we assume that modification of dependencies is not performance-critical.
//
namespace basis {

namespace dependency_error_space {

// The custom error space.
enum ErrorCode {
  // These are reserved errors.
  ERR_SUCCESS = 0,  // Success (default value). Same as OK.
  ERR_CANCELLED = 1,
  ERR_UNKNOWN = 2,
  ERR_PERMISSION_DENIED = 7,
  ERR_FAILED_PRECONDITION = 9,
  ERR_ABORTED = 10,
  ERR_OUT_OF_RANGE = 11,
  ERR_UNIMPLEMENTED = 12,
  ERR_INTERNAL = 13,
  ERR_DATA_LOSS = 15,
  ERR_UNAUTHENTICATED = 16,

  // The following errors start from 500,
  // to make sure they are not conflicting with the
  // canonical errors. DO NOT USE ANY VALUE BELOW 500 FOR THE ERRORS BEYOND
  // THIS LINE.
  ERR_CIRCULAR_DEPENDENCY = 500,
  ERR_DEPENDENCY_NOT_FOUND = 501,            // Entry (e.g. flow) not found.
};

// returns the singleton instance to be used through out the code.
const ::basis::ErrorSpace* ErrorSpace();

}  // namespace dependency_error_space

// Allow using status_macros. For example:
// return MAKE_ERROR(ERR_UNKNOWN) << "test";
namespace status_macros {

template <>
class ErrorCodeOptions<dependency_error_space::ErrorCode>
    : public BaseErrorCodeOptions {
 public:
  const ::basis::ErrorSpace* GetErrorSpace() {
    return dependency_error_space::ErrorSpace();
  }
};

}  // namespace status_macros

class Dependency;

// Use with functionality that can have dependencies,
// but can not be used as dependency.
//
// Represents one level of dependency hierarchy
// i.e. does not store nested levels in `storage_`.
//
class Dependencies final
  : public base::RefCountedThreadSafe<Dependencies>
{
 public:
  using Storage = std::set<scoped_refptr<Dependency>>;

  Dependencies();

  MUST_USE_RETURN_VALUE
  basis::Status addDependency(scoped_refptr<Dependency>);

  MUST_USE_RETURN_VALUE
  basis::Status removeDependency(scoped_refptr<Dependency>);

  // copy multiple dependencies
  MUST_USE_RETURN_VALUE
  basis::Status addDependencies(scoped_refptr<Dependencies>);

  // remove multiple dependencies
  MUST_USE_RETURN_VALUE
  basis::Status removeDependencies(scoped_refptr<Dependencies>);

  MUST_USE_RETURN_VALUE
  bool hasNestedDependency(scoped_refptr<Dependency> dependency) const;

  // Given dependency hierarchy:
  // A -> B -> D
  //      B -> C -> D
  // flatten(A) = [A,B,C,D]
  MUST_USE_RETURN_VALUE
  std::vector<scoped_refptr<Dependency>> flatten() const;

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const Storage& storage() const
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return storage_;
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  const size_t size() const
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return storage_.size();
  }

  friend class ::base::RefCountedThreadSafe<Dependencies>;

 private:
  /// \note from |base::RefCounted| docs:
  /// You should always make your destructor non-public,
  /// to avoid any code deleting
  /// the object accidently while there are references to it.
  ~Dependencies();

  Storage storage_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(Dependencies);
};

// Use with functionality that both can have dependencies
// and be used as dependency.
//
// Any `Dependency` may depend on other `Dependency` (zero or multiple)
//
class Dependency final
  : public base::RefCountedThreadSafe<Dependency>
{
 public:
  // Create with zero dependencies.
  // You can change dependencies after construction.
  Dependency();

  /// \note does nothing if dependency already exists
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  basis::Status addDependency(scoped_refptr<Dependency> dependency)
  {
    using namespace basis::dependency_error_space;
    using namespace basis::error;

    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK(dependency);
    if(!dependency) {
      RETURN_ERROR(INVALID_ARGUMENT)
        << "null can not be dependency";
    }

    // A -> A
    if(dependency == ::base::WrapRefCounted(this)) {
      RETURN_ERROR(ERR_CIRCULAR_DEPENDENCY)
        << "Detected circular dependency on self";
    }

    // A -> B -> C -> A
    if(dependency->hasNestedDependency(::base::WrapRefCounted(this))) {
      RETURN_ERROR(ERR_CIRCULAR_DEPENDENCY)
        << "Circular dependency detected";
    }

    DCHECK(dependencies_);
    return dependencies_->addDependency(dependency);
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  basis::Status removeDependency(scoped_refptr<Dependency> dependency)
  {
    using namespace basis::error;

    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK(dependency);
    if(!dependency) {
      RETURN_ERROR(INVALID_ARGUMENT)
        << "null can not be dependency";
    }

    if(dependency == ::base::WrapRefCounted(this)) {
      LOG(FATAL) << "Can not remove self from dependencies";
    }

    DCHECK(dependencies_);
    return dependencies_->removeDependency(dependency);
  }

  // copy multiple dependencies
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  basis::Status addDependencies(scoped_refptr<Dependencies> other)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK(other);

    DCHECK(dependencies_);
    return dependencies_->addDependencies(other);
  }

  // remove multiple dependencies
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  basis::Status removeDependencies(scoped_refptr<Dependencies> other)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK(other);

    DCHECK(dependencies_);
    return dependencies_->removeDependencies(other);
  }

  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  scoped_refptr<Dependencies> dependencies() const
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(dependencies_);
    return dependencies_;
  }

  /// \note iterates both top level dependencies
  /// and all nested (child) dependencies
  MUST_USE_RETURN_VALUE
  ALWAYS_INLINE
  bool hasNestedDependency(scoped_refptr<Dependency> dependency) const
  {
    using namespace basis::error;

    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK_FUNCTION_RECURSION(dependsOnRecursionLimit);

    DCHECK(dependency);
    if(!dependency || dependency == ::base::WrapRefCounted(this)) {
      return false;
    }

    DCHECK(dependencies_);
    return dependencies_->hasNestedDependency(dependency);
  }

  // Given dependency hierarchy:
  // A -> B -> D
  //      B -> C -> D
  // flatten(A) = [A,B,C,D]
  MUST_USE_RETURN_VALUE
  std::vector<scoped_refptr<Dependency>> flatten() const
  {
    DCHECK(dependencies_);
    std::vector<scoped_refptr<Dependency>> result{::base::WrapRefCounted(const_cast<Dependency*>(this))};
    auto flattenedChildren = dependencies_->flatten();
    // preallocate memory
    result.reserve(result.size() + flattenedChildren.size());
    result.insert(result.end(), flattenedChildren.begin(), flattenedChildren.end());
    return result;
  }

  friend class ::base::RefCountedThreadSafe<Dependency>;

 private:
  /// \note from |base::RefCounted| docs:
  /// You should always make your destructor non-public,
  /// to avoid any code deleting
  /// the object accidently while there are references to it.
  ~Dependency();

 private:
  mutable FUNCTION_RECURSION_CHECKER_LIMIT_999(dependsOnRecursionLimit);

  scoped_refptr<Dependencies> dependencies_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(Dependency);
};

}  // namespace basis
