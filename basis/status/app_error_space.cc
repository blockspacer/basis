#include "basis/status/app_error_space.hpp" // IWYU pragma: associated

#include <string>

namespace app_error_space {

namespace error {

static const char kErrorSpaceName[] = "app_error_space::ErrorSpace";

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
    case ERR_TABLE_FULL:
      return "ERR_TABLE_FULL";
    case ERR_TABLE_EMPTY:
      return "ERR_TABLE_EMPTY";
    case ERR_HARDWARE_ERROR:
      return "ERR_HARDWARE_ERROR";
    case ERR_INVALID_PARAM:
      return "ERR_INVALID_PARAM";
    case ERR_ENTRY_NOT_FOUND:
      return "ERR_ENTRY_NOT_FOUND";
    case ERR_ENTRY_EXISTS:
      return "ERR_ENTRY_EXISTS";
    case ERR_OPER_NOT_SUPPORTED:
      return "ERR_OPER_NOT_SUPPORTED";
    case ERR_OPER_DISABLED:
      return "ERR_OPER_DISABLED";
    case ERR_OPER_TIMEOUT:
      return "ERR_OPER_TIMEOUT";
    case ERR_OPER_STILL_RUNNING:
      return "ERR_OPER_STILL_RUNNING";
    case ERR_REBOOT_REQUIRED:
      return "ERR_REBOOT_REQUIRED";
    case ERR_FEATURE_UNAVAILABLE:
      return "ERR_FEATURE_UNAVAILABLE";
    case ERR_NOT_INITIALIZED:
      return "ERR_NOT_INITIALIZED";
    case ERR_NO_RESOURCE:
      return "ERR_NO_RESOURCE";
    case ERR_FILE_NOT_FOUND:
      return "ERR_FILE_NOT_FOUND";
    case ERR_AT_LEAST_ONE_OPER_FAILED:
      return "ERR_AT_LEAST_ONE_OPER_FAILED";
    case ERR_INVALID_INFO:
      return "ERR_INVALID_INFO";
    case ERR_NO_OP:
      return "ERR_NO_OP";
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
    case ERR_UNAUTHENTICATED:
      return ::basis::error::UNAUTHENTICATED;
    case ERR_INTERNAL:
    case ERR_HARDWARE_ERROR:
      return ::basis::error::INTERNAL;
    case ERR_INVALID_PARAM:
    case ERR_INVALID_INFO:
      return ::basis::error::INVALID_ARGUMENT;
    case ERR_OPER_TIMEOUT:
      return ::basis::error::DEADLINE_EXCEEDED;
    case ERR_ENTRY_NOT_FOUND:
      return ::basis::error::NOT_FOUND;
    case ERR_ENTRY_EXISTS:
      return ::basis::error::ALREADY_EXISTS;
    case ERR_UNIMPLEMENTED:
    case ERR_OPER_NOT_SUPPORTED:
    case ERR_OPER_DISABLED:
      return ::basis::error::UNIMPLEMENTED;
    case ERR_FEATURE_UNAVAILABLE:
      return ::basis::error::UNAVAILABLE;
    case ERR_NO_RESOURCE:
      return ::basis::error::RESOURCE_EXHAUSTED;
    case ERR_FAILED_PRECONDITION:
    case ERR_NOT_INITIALIZED:
      return ::basis::error::FAILED_PRECONDITION;
    case ERR_OUT_OF_RANGE:
    case ERR_TABLE_FULL:
    case ERR_TABLE_EMPTY:
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
    app_error_space::ErrorSpace();

}  // namespace app_error_space
