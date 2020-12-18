#include "basis/dependency_hierarchy/dependency_error_space.hpp" // IWYU pragma: associated

#include <base/logging.h>

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

}  // namespace basis
