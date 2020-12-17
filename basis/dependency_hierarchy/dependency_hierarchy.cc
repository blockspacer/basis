#include "basis/dependency_hierarchy/dependency_hierarchy.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/i18n/icu_util.h>

#include <algorithm>

namespace basis {

namespace dependency_error_space {

namespace error {

static const char kErrorSpaceName[] = "dependency_error_space::ErrorSpace";

static const char kErrorUnknownStr[] = "UNKNOWN";

::std::string ErrorCode_Name(const ErrorCode code)
{
  switch (code) {
    case ERR_SUCCESS:
      return "ERR_SUCCESS";
    case ERR_CANCELLED:
      return "ERR_CANCELLED";
    case ERR_UNKNOWN:
      return "ERR_UNKNOWN";
    case ERR_PERMISSION_DENIED:
      return "ERR_PERMISSION_DENIED";
    case ERR_FAILED_PRECONDITION:
      return "ERR_FAILED_PRECONDITION";
    case ERR_ABORTED:
      return "ERR_ABORTED";
    case ERR_OUT_OF_RANGE:
      return "ERR_OUT_OF_RANGE";
    case ERR_UNIMPLEMENTED:
      return "ERR_UNIMPLEMENTED";
    case ERR_INTERNAL:
      return "ERR_INTERNAL";
    case ERR_DATA_LOSS:
      return "ERR_DATA_LOSS";
    case ERR_UNAUTHENTICATED:
      return "ERR_UNAUTHENTICATED";
    case ERR_CIRCULAR_DEPENDENCY:
      return "ERR_CIRCULAR_DEPENDENCY";
    case ERR_DEPENDENCY_NOT_FOUND:
      return "ERR_DEPENDENCY_NOT_FOUND";
  }

  // No default clause, clang will abort if a code is missing from
  // above switch.
  return kErrorUnknownStr;
}

::basis::error::Code ErrorCode_Canonical(const ErrorCode code)
{
  switch (code) {
    case ERR_SUCCESS:
      return ::basis::error::OK;
    case ERR_CANCELLED:
      return ::basis::error::CANCELLED;
    case ERR_UNKNOWN:
      return ::basis::error::UNKNOWN;
    case ERR_PERMISSION_DENIED:
      return ::basis::error::PERMISSION_DENIED;
    case ERR_ABORTED:
      return ::basis::error::ABORTED;
    case ERR_DATA_LOSS:
      return ::basis::error::DATA_LOSS;
    case ERR_INTERNAL:
      return ::basis::error::INTERNAL;
    case ERR_DEPENDENCY_NOT_FOUND:
      return ::basis::error::NOT_FOUND;
    case ERR_UNIMPLEMENTED:
      return ::basis::error::UNIMPLEMENTED;
    case ERR_FAILED_PRECONDITION:
      return ::basis::error::FAILED_PRECONDITION;
    case ERR_OUT_OF_RANGE:
    case ERR_CIRCULAR_DEPENDENCY:
      return ::basis::error::OUT_OF_RANGE;
    default:
      return ::basis::error::UNKNOWN;  // Default error.
  }
}

bool ErrorCode_IsValid(const ErrorCode code)
{
  return ErrorCode_Name(code) != kErrorUnknownStr;
}

class ErrorSpace : public ::basis::ErrorSpace {
 public:
  ErrorSpace() : ::basis::ErrorSpace(kErrorSpaceName) {}
  ~ErrorSpace() override {}

  ::std::string String(int code) const final {
    if (!ErrorCode_IsValid(static_cast<ErrorCode>(code))) {
      code = ERR_UNKNOWN;
    }
    return ErrorCode_Name(static_cast<ErrorCode>(code));
  }

  // map custom error code to canonical error code
  ::basis::error::Code CanonicalCode(const ::basis::Status& status) const final {
    return ErrorCode_Canonical(static_cast<ErrorCode>(status.error_code()));
  }

  // ErrorSpace is neither copyable nor movable.
  ErrorSpace(const ErrorSpace&) = delete;
  ErrorSpace& operator=(const ErrorSpace&) = delete;
};

}  // namespace error

// Singleton ErrorSpace.
const ::basis::ErrorSpace* ErrorSpace() {
  static const ::basis::ErrorSpace* space = new error::ErrorSpace();
  return space;
}

// Force registering of the errorspace at run-time.
static const ::basis::ErrorSpace* dummy __attribute__((unused)) =
    dependency_error_space::ErrorSpace();

}  // namespace dependency_error_space

Dependency::Dependency()
  : dependencies_(base::MakeRefCounted<Dependencies>())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

Dependency::~Dependency()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

Dependencies::Dependencies()
{}

Dependencies::~Dependencies()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for(scoped_refptr<Dependency> dependency: storage_) {
    DCHECK(dependency);
  }
}

basis::Status Dependencies::addDependency(scoped_refptr<Dependency> dependency)
{
  using namespace basis::error;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(dependency);
  if(!dependency) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  storage_.emplace(dependency);

  RETURN_OK();
}

basis::Status Dependencies::removeDependency(scoped_refptr<Dependency> dependency)
{
  using namespace basis::error;
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(dependency);
  if(!dependency) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  auto found = std::find(storage_.begin(), storage_.end(), dependency);
  if (found == storage_.end()) {
    RETURN_ERROR(ERR_DEPENDENCY_NOT_FOUND).without_logging()
      << "Can not remove dependency that was not added before";
  }

  storage_.erase(found);

  RETURN_OK();
}

basis::Status Dependencies::addDependencies(scoped_refptr<Dependencies> other)
{
  using namespace basis::error;
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(other);
  if(!other) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  for (scoped_refptr<Dependency> dependency: other->storage()) {
    DCHECK(dependency);
    basis::Status result = addDependency(dependency);
    /// \note skips minor errors (`INVALID_ARGUMENT`, etc.)
    RETURN_IF_ERROR_CODE(result, ERR_CIRCULAR_DEPENDENCY);
  }

  RETURN_OK();
}

basis::Status Dependencies::removeDependencies(scoped_refptr<Dependencies> other)
{
  using namespace basis::error;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(other);
  if(!other) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  basis::Status result(FROM_HERE);

  for (scoped_refptr<Dependency> dependency: other->storage()) {
    DCHECK(dependency);
    APPEND_STATUS_IF_ERROR(result, removeDependency(dependency));
  }

  return result;
}

std::vector<scoped_refptr<Dependency>> Dependencies::flatten() const
{
  std::vector<scoped_refptr<Dependency>> result;

  // will store dependencies at some hierarchy level
  std::set<scoped_refptr<Dependencies>> dependenciesToProcess{
    ::base::WrapRefCounted(const_cast<Dependencies*>(this))};

  /// \note recursion replaced with iteration
  do {
    std::set<scoped_refptr<Dependencies>> dependenciesToAdd;
    // iterate elements in current hierarchy level
    for(auto dep_it = dependenciesToProcess.begin(); dep_it != dependenciesToProcess.end(); dep_it++)
    {
      DCHECK(*dep_it);

      const Dependencies::Storage& storage = (*dep_it)->storage();
      // process dependencies from element in current hierarchy level
      for(auto store_it = storage.begin(); store_it != storage.end(); store_it++)
      {
        DCHECK(*store_it);

        result.push_back(*store_it);

        // add dependencies from element in nested hierarchy level
        if((*store_it)->dependencies()) {
          dependenciesToAdd.insert((*store_it)->dependencies());
        }
      }
    }
    // reset dependencies to dependencies from nested hierarchy levels
    {
      dependenciesToProcess.clear();
      dependenciesToProcess = dependenciesToAdd;
    }
  } while (!dependenciesToProcess.empty());

  return result;
}

bool Dependencies::hasNestedDependency(scoped_refptr<Dependency> dependency) const
{
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(dependency);
  if(!dependency) {
    return false;
  }

  // will store dependencies at some hierarchy level
  std::set<scoped_refptr<Dependencies>> dependenciesToProcess{
    ::base::WrapRefCounted(const_cast<Dependencies*>(this))};

  /// \note recursion replaced with iteration
  do {
    std::set<scoped_refptr<Dependencies>> dependenciesToAdd;
    // iterate elements in current hierarchy level
    for(auto dep_it = dependenciesToProcess.begin(); dep_it != dependenciesToProcess.end(); dep_it++)
    {
      DCHECK(*dep_it);

      const Dependencies::Storage& storage = (*dep_it)->storage();
      // process dependencies from element in current hierarchy level
      for(auto store_it = storage.begin(); store_it != storage.end(); store_it++)
      {
        DCHECK(*store_it);

        if(*store_it == dependency) {
          return true;
        }

        // add dependencies from element in nested hierarchy level
        if((*store_it)->dependencies()) {
          dependenciesToAdd.insert((*store_it)->dependencies());
        }
      }
    }
    // reset dependencies to dependencies from nested hierarchy levels
    {
      dependenciesToProcess.clear();
      dependenciesToProcess = dependenciesToAdd;
    }
  } while (!dependenciesToProcess.empty());

  return false;
}

}  // namespace basis
