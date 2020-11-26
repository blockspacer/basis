// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


// This library contains helper macros and methods to make returning errors
// and propagating statuses easier.
//
// We use ::basis::Status for error codes.  Methods that return status should
// have signatures like
//   ::basis::Status Method(arg, ...);
// or
//   ::basis::StatusOr<ValueType> Method(arg, ...);
//
// Inside the method, to return errors, use the macros
//   RETURN_ERROR() << "Message with ::basis::error::UNKNOWN code";
//   RETURN_ERROR(code_enum)
//       << "Message with an error code, in that error_code's ErrorSpace "
//       << "(See ErrorCodeOptions below)";
//   RETURN_ERROR(error_space, code_int)
//       << "Message with integer error code in specified ErrorSpace "
//       << "(Not recommended - use previous form with an enum code instead)";
//
// When calling another method, use this to propagate status easily.
//   RETURN_IF_ERROR(method(args));
//
// Use this to also append to the end of the error message when propagating
// an error:
//   RETURN_IF_ERROR_WITH_APPEND(method(args)) << " for method with " << args;
//
// Use this to propagate the status to a Stubby1 or Stubby2 RPC easily. This
// assumes an AutoClosureRunner has been set up on the RPC's associated
// closure, or it gets run some other way to signal the RPC's termination.
//   RETURN_RPC_IF_ERROR(rpc, method(args));
//
// Use this to propagate the status to a ::basis::Task* instance
// calling task->Return() with the status.
//   RETURN_TASK_IF_ERROR(task, method(args));
//
// For StatusOr results, you can extract the value or return on error.
//   ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(arg));
// Or:
//   ValueType value;
//   ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
//
// WARNING: ASSIGN_OR_RETURN expands into multiple statements; it cannot be used
//  in a single statement (e.g. as the body of an if statement without {})!
//
// This can optionally be used to return ::basis::Status::OK.
//   RETURN_OK();
//
// To construct an error without immediately returning it, use MAKE_ERROR,
// which supports the same argument types as RETURN_ERROR.
//   ::basis::Status status = MAKE_ERROR(...) << "Message";
//
// To add additional text onto an existing error, use
//   ::basis::Status new_status = APPEND_ERROR(status) << ", additional details";
//
// These macros can also be assigned to a ::basis::StatusOr variable:
//   ::basis::StatusOr<T> status_or = MAKE_ERROR(...) << "Message";
//
// They can also be used to return from a function that returns
// ::basis::StatusOr:
//   ::basis::StatusOr<T> MyFunction() {
//     RETURN_ERROR(...) << "Message";
//   }
//
//
// Error codes:
//
// Using error codes is optional.  ::basis::error::UNKNOWN will be used if no
// code is provided.
//
// By default, these macros work with canonical ::basis::error::Code codes,
// using the canonical ErrorSpace. These macros will also work with
// project-specific ErrorSpaces and error code enums if a specialization
// of ErrorCodeOptions is defined.
//
//
// Logging:
//
// RETURN_ERROR and MAKE_ERROR log the error to LOG(ERROR) by default.
//
// Logging can be turned on or off for a specific error by using
//   RETURN_ERROR().with_logging() << "Message logged to LOG(ERROR)";
//   RETURN_ERROR().without_logging() << "Message not logged";
//   RETURN_ERROR().set_logging(false) << "Message not logged";
//   RETURN_ERROR().severity(INFO) << "Message logged to LOG(INFO)";
//
// If logging is enabled, this will make an error also log a stack trace.
//   RETURN_ERROR().with_log_stack_trace() << "Message";
//
// Logging can also be controlled within a scope using
// ScopedErrorLogSuppression.
//
//
// Assertion handling:
//
// When you would use a CHECK, CHECK_EQ, etc, you can instead use RET_CHECK
// to return a ::basis::Status if the condition is not met:
//   RET_CHECK(ptr != null);
//   RET_CHECK_GT(value, 0) << "Optional additional message";
//   RET_CHECK_FAIL() << "Always fail, like a LOG(FATAL)";
//
// These are a better replacement for CHECK because they don't crash, and for
// DCHECK and LOG(DFATAL) because they don't ignore errors in opt builds.
//
// The RET_CHECK* macros can only be used in functions that return
// ::basis::Status.
//
// The returned error will have the ::basis::error::INTERNAL error code and the
// message will include the file and line number.  The current stack trace
// will also be logged.

#pragma once

#include "basis/status/status.hpp"
#include "basis/status/statusor.hpp"

#include <memory>
#include <ostream>
#include <sstream> // IWYU pragma: keep
#include <string>
#include <vector>

#include <base/logging.h>
#include <base/macros.h>
#include <base/location.h>
#include <base/compiler_specific.h>

namespace basis {

namespace status_macros {

using logging::LogSeverity;

bool IsMacroErrorLoggedByDefault();

/// \note logs even if `IsMacroErrorLoggedByDefault` disabled.
/// Use for important errors that must always be logged.
//
// USAGE
//
// LOG_IF_ERROR(statusor.status());
// LOG_IF_ERROR(statusor.status(), ERROR, /*should_log_stack_trace*/ true);
//
#define LOG_IF_ERROR(expr, ...) \
  do {                                                                         \
    /* Using _status below to avoid capture problems if expr is "status". */   \
    const ::basis::Status _status = (expr);                                    \
    if (UNLIKELY(!_status.ok())) {                                             \
      ::basis::status_macros::LogError(_status, FROM_HERE, ##__VA_ARGS__);  \
    }                                                                          \
  } while (0)

/// \note logs even if `IsMacroErrorLoggedByDefault` disabled
/// Use for important errors that must always be logged.
//
// Run a command that returns a ::basis::Status.  If the called code returns an
// error status, return that status up out of this method too.
//
// Example:
//   RETURN_IF_ERROR(DoThings(4));
#define RETURN_AND_LOG_IF_ERROR(expr)                                                \
  LOG_IF_ERROR(expr); \
  RETURN_IF_ERROR(expr)

/// \note logs even if `IsMacroErrorLoggedByDefault` disabled
/// Use for important errors that must always be logged.
//
#define RETURN_AND_LOG_IF_ERROR_WITH_APPEND(expr)                                     \
  LOG_IF_ERROR(expr); \
  RETURN_IF_ERROR_WITH_APPEND(expr)

/// \note logs even if `IsMacroErrorLoggedByDefault` disabled
/// Use for important errors that must always be logged.
//
#define ASSIGN_OR_RETURN_AND_LOG(lhs, rexpr)                                     \
  LOG_IF_ERROR(rexpr); \
  ASSIGN_OR_RETURN(lhs, rexpr)

void LogError(const ::basis::Status& status
  , const ::base::Location location
  , LogSeverity log_severity = ::logging::LOG_ERROR
  , bool should_log_stack_trace = true);

// Base class for options attached to a project-specific error code enum.
// Projects that use non-canonical error codes should specialize the
// ErrorCodeOptions template below with a subclass of this class, overriding
// GetErrorSpace, and optionally other methods.
class BaseErrorCodeOptions {
 public:
  // Return the ErrorSpace to use for this error code enum.
  // Not implemented in base class - must be overridden.
  const ::basis::ErrorSpace* GetErrorSpace();

  // Returns true if errors with this code should be logged upon creation, by
  // default.  (Default can be overridden with modifiers on MakeErrorStream.)
  // Can be overridden to customize default logging per error code.
  bool IsLoggedByDefault(int code) { return IsMacroErrorLoggedByDefault(); }
};

// Template that should be specialized for any project-specific error code enum.
template <typename ERROR_CODE_ENUM_TYPE>
class ErrorCodeOptions;

// Specialization for the canonical error codes and canonical ErrorSpace.
template <>
class ErrorCodeOptions< ::basis::error::Code> : public BaseErrorCodeOptions {
 public:
  const ::basis::ErrorSpace* GetErrorSpace() {
    return ::basis::Status::canonical_space();
  }
};

// Stream object used to collect error messages in MAKE_ERROR macros or
// append error messages with APPEND_ERROR.
// It accepts any arguments with operator<< to build an error string, and
// then has an implicit cast operator to ::basis::Status, which converts the
// logged string to a Status object and returns it, after logging the error.
// At least one call to operator<< is required; a compile time error will be
// generated if none are given. Errors will only be logged by default for
// certain status codes, as defined in IsLoggedByDefault. This class will
// give DFATAL errors if you don't retrieve a ::basis::Status exactly once before
// destruction.
//
// The class converts into an intermediate wrapper object
// MakeErrorStreamWithOutput to check that the error stream gets at least one
// item of input.
class MakeErrorStream {
 public:
  // Wrapper around MakeErrorStream that only allows for output. This is created
  // as output of the first operator<< call on MakeErrorStream. The bare
  // MakeErrorStream does not have a ::basis::Status operator. The net effect of
  // that is that you have to call operator<< at least once or else you'll get
  // a compile time error.
  class MakeErrorStreamWithOutput {
   public:
    explicit MakeErrorStreamWithOutput(MakeErrorStream* error_stream)
        : wrapped_error_stream_(error_stream) {}

    template <typename T>
    MakeErrorStreamWithOutput& operator<<(const T& value) {
      *wrapped_error_stream_ << value;
      return *this;
    }

    // Implicit cast operators to ::basis::Status and ::basis::StatusOr.
    // Exactly one of these must be called exactly once before destruction.
    operator ::basis::Status() {
      return wrapped_error_stream_->GetStatus();
    }
    template <typename T>
    operator ::basis::StatusOr<T>() {
      return wrapped_error_stream_->GetStatus();
    }

    // MakeErrorStreamWithOutput is neither copyable nor movable.
    MakeErrorStreamWithOutput(const MakeErrorStreamWithOutput&) = delete;
    MakeErrorStreamWithOutput& operator=(const MakeErrorStreamWithOutput&) =
        delete;

   private:
    MakeErrorStream* wrapped_error_stream_;
  };

  // Make an error with ::basis::error::UNKNOWN.
  MakeErrorStream(const base::Location& location)
      : impl_(new Impl(location,
                       ::basis::Status::canonical_space(),
                       ::basis::error::UNKNOWN, this)) {}

  // Make an error with the given error code and error_space.
  MakeErrorStream(const ::base::Location& location,
                  const ::basis::ErrorSpace* error_space, int code)
      : impl_(new Impl(location, error_space, code, this)) {}

  // Make an error that appends additional messages onto a copy of status.
  MakeErrorStream(::basis::Status status, const ::base::Location& location)
      : impl_(new Impl(status, location, this)) {}

  // Make an error with the given code, inferring its ErrorSpace from
  // code's type using the specialized ErrorCodeOptions.
  template <typename ERROR_CODE_TYPE>
  MakeErrorStream(const ::base::Location& location, ERROR_CODE_TYPE code)
    : impl_(new Impl(
          location,
          ErrorCodeOptions<ERROR_CODE_TYPE>().GetErrorSpace(),
          code, this,
          ErrorCodeOptions<ERROR_CODE_TYPE>().IsLoggedByDefault(code))) {}

  template <typename T>
  MakeErrorStreamWithOutput& operator<<(const T& value) {
    CheckNotDone();
    impl_->stream_ << value;
    return impl_->make_error_stream_with_output_wrapper_;
  }

  // Disable sending this message to LOG(ERROR), even if this code is usually
  // logged. Some error codes are logged by default, and others are not.
  // Usage:
  //   return MAKE_ERROR().without_logging() << "Message";
  MakeErrorStream& without_logging() {
    impl_->should_log_ = false;
    return *this;
  }

  // Send this message to LOG(ERROR), even if this code is not usually logged.
  // Usage:
  //   return MAKE_ERROR().with_logging() << "Message";
  MakeErrorStream& with_logging() {
    impl_->should_log_ = true;
    return *this;
  }

  // Determine whether to log this message based on the value of <should_log>.
  MakeErrorStream& set_logging(bool should_log) {
    impl_->should_log_ = should_log;
    return *this;
  }

  // Log the status at this LogSeverity: INFO, WARNING, or ERROR.
  // Setting severity to NUM_SEVERITIES will disable logging.
  MakeErrorStream& severity(LogSeverity log_severity) {
    impl_->log_severity_ = log_severity;
    return *this;
  }

  // When this message is logged (see with_logging()), include the stack trace.
  MakeErrorStream& with_log_stack_trace() {
    impl_->should_log_stack_trace_ = true;
    return *this;
  }

  // When this message is logged, omit the stack trace, even if
  // with_log_stack_trace() was previously called.
  MakeErrorStream& without_log_stack_trace() {
    impl_->should_log_stack_trace_ = false;
    return *this;
  }

  // Adds RET_CHECK failure text to error message.
  MakeErrorStreamWithOutput& add_ret_check_failure(const char* condition) {
    return *this << "RET_CHECK failure (" << impl_->location_.ToString()
                 << ") " << condition << " ";
  }

  // Adds RET_CHECK_FAIL text to error message.
  MakeErrorStreamWithOutput& add_ret_check_fail_failure() {
    return *this << "RET_CHECK_FAIL failure (" << impl_->location_.ToString() << ") ";
  }

  // MakeErrorStream is neither copyable nor movable.
  MakeErrorStream(const MakeErrorStream&) = delete;
  MakeErrorStream& operator=(const MakeErrorStream&) = delete;

 private:
  class Impl {
   public:
    Impl(const ::base::Location& location,
         const ::basis::ErrorSpace* error_space, int  code,
         MakeErrorStream* error_stream,
         bool is_logged_by_default = IsMacroErrorLoggedByDefault());
    Impl(const ::basis::Status& status, const ::base::Location& location,
         MakeErrorStream* error_stream);

    ~Impl();

    // This must be called exactly once before destruction.
    ::basis::Status GetStatus();

    void CheckNotDone() const;

    // Impl is neither copyable nor movable.
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

   private:
    const ::base::Location location_;
    const ::basis::ErrorSpace* error_space_;
    int code_;

    std::string prior_message_;
    bool is_done_;  // true after Status object has been returned
    std::ostringstream stream_;
    bool should_log_;
    LogSeverity log_severity_;
    bool should_log_stack_trace_;

    // Wrapper around the MakeErrorStream object that has a ::basis::Status
    // conversion. The first << operator called on MakeErrorStream will return
    // this object, and only this object can implicitly convert to
    // ::basis::Status. The net effect of this is that you'll get a compile time
    // error if you call MAKE_ERROR etc. without adding any output.
    MakeErrorStreamWithOutput make_error_stream_with_output_wrapper_;

    friend class MakeErrorStream;
  };

  void CheckNotDone() const;

  // Returns the status. Used by MakeErrorStreamWithOutput.
  ::basis::Status GetStatus() const { return impl_->GetStatus(); }

  // Store the actual data on the heap to reduce stack frame sizes.
  std::unique_ptr<Impl> impl_;
};

// Make an error ::basis::Status, building message with LOG-style shift
// operators.  The error also gets sent to LOG(ERROR).
//
// Takes an optional error code parameter. Uses ::basis::error::UNKNOWN by
// default.  Returns a ::basis::Status object that must be returned or stored.
//
// Examples:
//   return MAKE_ERROR() << "Message";
//   return MAKE_ERROR(INTERNAL_ERROR) << "Message";
//   ::basis::Status status = MAKE_ERROR() << "Message";
#define MAKE_ERROR(...) \
  ::basis::status_macros::MakeErrorStream(FROM_HERE, ##__VA_ARGS__)

// accepts custom |base::Location| i.e. |from_here|
#define MAKE_ERROR_HERE(from_here, ...) \
  ::basis::status_macros::MakeErrorStream(from_here, ##__VA_ARGS__)

// Return a new error based on an existing error,
// with an additional string appended.
// Otherwise behaves like MAKE_ERROR,
// including logging the error by default.
// Requires !status.ok().
// Example:
//   status = APPEND_ERROR(status) << ", more details";
//   return APPEND_ERROR(status) << ", more details";
#define APPEND_ERROR(status) \
  ::basis::status_macros::MakeErrorStream((status), FROM_HERE)

#define RETURN_APPEND_ERROR(status) \
  DCHECK(!statusor.ok()) << "APPEND_ERROR requires !status.ok()."; \
  return APPEND_ERROR(status)

// Shorthand to make an error (with MAKE_ERROR) and return it.
//   if (error) {
//     RETURN_ERROR() << "Message";
//   }
#define RETURN_ERROR return MAKE_ERROR

// Return success.
#define RETURN_OK() \
  return ::basis::OkStatus(FROM_HERE)

// A macro for simplify checking and logging a condition. The error code
// return here is the one that matches the most of the uses.
#define RETURN_ERR_IF_FALSE(cond, ...) \
  if (LIKELY(cond)) {    \
  } else /* NOLINT */               \
    return MAKE_ERROR(__VA_ARGS__) << "'" << #cond << "' is false. "

// A simple class to explicitly cast the return value of an ::basis::Status
// to bool.
class BooleanStatus {
 public:
  BooleanStatus(::basis::Status status) : status_(status) {}  // NOLINT
  // Implicitly cast to bool.
  operator bool() const { return status_.ok(); }
  inline ::basis::Status status() const { return status_; }
 private:
  ::basis::Status status_;
};

inline const std::string FixMessage(const std::string& msg) {
  std::string str = msg;
  std::size_t found = str.find_last_not_of(" \t\f\v\n\r");
  if (found != std::string::npos) {
    str.erase(found + 1);
    if (str.back() != '.' && str.back() != '!' && str.back() != '?' &&
        str.back() != ';' && str.back() != ':' && str.back() != ',') {
      str += ". ";
    } else {
      str += " ";
    }
  } else {
    str.clear();
  }

  return str;
}

// A macro for simplifying creation of a new error or appending new info to an
// error based on the return value of a function that returns ::basis::Status.
#define APPEND_STATUS_IF_ERROR(out, expr)                                      \
  if (const ::basis::status_macros::BooleanStatus __ret = (expr)) {                                    \
  } else /* NOLINT */                                                          \
    out = APPEND_ERROR(!out.ok() ? out : __ret.status().StripMessage())        \
              .without_logging()                                               \
          << (out.error_message().empty() || out.error_message().back() == ' ' \
                  ? ""                                                         \
                  : " ")                                                       \
          << ::basis::status_macros::FixMessage(__ret.status().error_message())

// Wraps a ::basis::Status so it can be assigned and used in an if-statement.
// Implicitly converts from status and to bool.
namespace internal {
class UtilStatusConvertibleToBool {
 public:
  // Implicity conversion from a status to wrap.
  // Need implicit conversion to allow in if-statement.
  // NOLINTNEXTLINE(runtime/explicit)
  UtilStatusConvertibleToBool(::basis::Status status) : status_(status) {}
  // Implicity cast to bool. True on ok() and false on error.
  operator bool() const { return LIKELY(status_.ok()); }
  // Returns the wrapped status.
  ::basis::Status status() const { return status_; }

 private:
  ::basis::Status status_;
};
}  // namespace internal

#define RETURN_WITHOUT_LOG_IF_ERROR(expr)                                                \
  do {                                                                       \
    /* Using _status below to avoid capture problems if expr is "status". */ \
    const ::basis::Status _status = (expr);                                   \
    if (UNLIKELY(!_status.ok())) {                                 \
      return _status;                                                        \
    }                                                                        \
  } while (0)

/// \note performs extra logging using `LOG(ERROR)` only if `IsMacroErrorLoggedByDefault` enabled
//
// Run a command that returns a ::basis::Status.  If the called code returns an
// error status, return that status up out of this method too.
//
// Example:
//   RETURN_IF_ERROR(DoThings(4));
#define RETURN_IF_ERROR(expr)                                                \
  do {                                                                       \
    /* Using _status below to avoid capture problems if expr is "status". */ \
    const ::basis::Status _status = (expr);                                   \
    if (UNLIKELY(!_status.ok())) {                                 \
      if (UNLIKELY(::basis::status_macros::IsMacroErrorLoggedByDefault()))   \
        LOG(ERROR) << "Return Error: " << #expr << " failed with " << _status; \
      return _status;                                                        \
    }                                                                        \
  } while (0)

#define RETURN_WITHOUT_LOG_IF_ERROR_WITH_APPEND(expr)                                     \
  /* Using _status below to avoid capture problems if expr is "status". */    \
  /* We also need the error to be in the else clause, to avoid a dangling  */ \
  /* else in the client code. (see test for example). */                      \
  if (const ::basis::status_macros::internal::UtilStatusConvertibleToBool     \
          _status = (expr)) {                                                 \
  } else /* NOLINT */                                                         \
    return ::basis::status_macros::MakeErrorStream(_status.status(),          \
                                                  FROM_HERE)                  \
        .without_logging()

/// \note performs extra logging using `LOG(ERROR)` only if `IsMacroErrorLoggedByDefault` enabled
//
// This is like RETURN_IF_ERROR, but instead of propagating the existing error
// Status, it constructs a new Status and can append additional messages.
//
// This has slightly worse performance that RETURN_IF_ERROR in both OK and ERROR
// case. (see status_macros_benchmark.cc for details)
//
// Example:
//   RETURN_IF_ERROR_WITH_APPEND(DoThings(4)) << "Things went wrong for " << 4;
//
// Args:
//   expr: Command to run that returns a ::basis::Status.
#define RETURN_IF_ERROR_WITH_APPEND(expr)                                     \
  /* Using _status below to avoid capture problems if expr is "status". */    \
  /* We also need the error to be in the else clause, to avoid a dangling  */ \
  /* else in the client code. (see test for example). */                      \
  if (const ::basis::status_macros::internal::UtilStatusConvertibleToBool     \
          _status = (expr); !_status) {                                       \
    if (UNLIKELY(::basis::status_macros::IsMacroErrorLoggedByDefault()))      \
      LOG(ERROR) << "Return error: " << #expr << " failed with "              \
                    << _status.status();                                      \
  }                                                                           \
  if (const ::basis::status_macros::internal::UtilStatusConvertibleToBool     \
          _status = (expr)) {                                                 \
  } else                                                                      \
    return ::basis::status_macros::MakeErrorStream(_status.status(),          \
                                                  FROM_HERE)                  \
        .without_logging()

// Internal helper for concatenating macro values.
#define STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define STATUS_MACROS_CONCAT_NAME(x, y) STATUS_MACROS_CONCAT_NAME_INNER(x, y)

#define ASSIGN_OR_RETURN_WITHOUT_LOG_IMPL(statusor, lhs, rexpr)                       \
  auto statusor = (rexpr);                                                \
  if (UNLIKELY(!statusor.ok())) {                               \
    return statusor.status();                                             \
  }                                                                       \
  lhs = statusor.ConsumeValueOrDie();

/// \note performs extra logging using `LOG(ERROR)` only if `IsMacroErrorLoggedByDefault` enabled
//
#define ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr)                         \
  auto statusor = (rexpr);                                                  \
  if (UNLIKELY(!statusor.ok())) {                                           \
    if (UNLIKELY(::basis::status_macros::IsMacroErrorLoggedByDefault()))    \
      LOG(ERROR) << "Return Error: " << #rexpr << " at " << __FILE__ << ":" \
               << __LINE__;                                                 \
    return statusor.status();                                               \
  }                                                                         \
  lhs = statusor.ConsumeValueOrDie();

#define ASSIGN_OR_RETURN_WITHOUT_LOG(lhs, rexpr) \
  ASSIGN_OR_RETURN_WITHOUT_LOG_IMPL( \
      STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, rexpr);

/// \note performs extra logging using `LOG(ERROR)` only if `IsMacroErrorLoggedByDefault` enabled
//
// Executes an expression that returns a ::basis::StatusOr, extracting its value
// into the variable defined by lhs (or returning on error).
//
// Example: Declaring and initializing a new value
//   ASSIGN_OR_RETURN(const ValueType& value, MaybeGetValue(arg));
//
// Example: Assigning to an existing value
//   ValueType value;
//   ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
//
// Example: Assigning std::unique_ptr<T>
//   ASSIGN_OR_RETURN(std::unique_ptr<T> ptr, MaybeGetPtr(arg));
//
// The value assignment example would expand into:
//   StatusOr<ValueType> status_or_value = MaybeGetValue(arg);
//   if (!status_or_value.ok()) return status_or_value.status();
//   value = status_or_value.ConsumeValueOrDie();
//
// WARNING: ASSIGN_OR_RETURN expands into multiple statements; it cannot be used
//  in a single statement (e.g. as the body of an if statement without {})!
#define ASSIGN_OR_RETURN(lhs, rexpr) \
  ASSIGN_OR_RETURN_IMPL( \
      STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, rexpr);

// If condition is false, this macro returns, from the current function, a
// ::basis::Status with the ::basis::error::INTERNAL code.
// For example:
//   RET_CHECK(condition) << message;
// is equivalent to:
//   if(!condition) {
//     return MAKE_ERROR() << message;
//   }
// Note that the RET_CHECK macro includes some more information in the error
// and logs a stack trace.
//
// Intended to be used as a replacement for CHECK where crashes are
// unacceptable. The containing function must return a ::basis::Status.
#define RET_CHECK(condition)                                             \
  while (UNLIKELY(!(condition)))                               \
    while (::basis::status_macros::helper_log_always_return_true())       \
  return ::basis::status_macros::MakeErrorStream(FROM_HERE,      \
                                                ::basis::error::INTERNAL) \
      .with_log_stack_trace()                                            \
      .add_ret_check_failure(#condition)

///////
// Implementation code for RET_CHECK_EQ, RET_CHECK_NE, etc.

// Wraps a string*, allowing it to be written to an ostream and deleted.
// This is needed because the RET_CHECK_OP macro needs to free the memory
// after the error message is logged.
namespace internal {
struct ErrorDeleteStringHelper {
  explicit ErrorDeleteStringHelper(std::string* str_in) : str(str_in) { }
  ~ErrorDeleteStringHelper() { delete str; }
  std::string* str;

  // ErrorDeleteStringHelper is neither copyable nor movable.
  ErrorDeleteStringHelper(const ErrorDeleteStringHelper&) = delete;
  ErrorDeleteStringHelper& operator=(const ErrorDeleteStringHelper&) = delete;
};

}  // namespace internal

// Helper macros for binary operators.
// Don't use these macro directly in your code, use RET_CHECK_EQ et al below.

// The definition of RET_CHECK_OP is modeled after that of CHECK_OP_LOG in
// logging.h.

template<class t1, class t2>
std::string* MakeRetCheckOpString(
    const t1& v1, const t2& v2, const char* names) {
  std::stringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  return new std::string(ss.str());
}
#define DEFINE_RET_CHECK_OP_IMPL(name, op)                             \
  template <class t1, class t2>                                        \
  inline std::string* RetCheck##name##Impl(const t1& v1, const t2& v2, \
                                           const char* names) {        \
    if (LIKELY(v1 op v2)) {                                 \
      return NULL;                                                     \
    } else {                                                           \
      LOG(ERROR) << "Return Error: "                                   \
                 << " at " << __FILE__ << ":" << __LINE__;             \
      return MakeRetCheckOpString(v1, v2, names);                      \
    }                                                                  \
  }                                                                    \
  inline std::string* RetCheck##name##Impl(int v1, int v2,             \
                                           const char* names) {        \
    return RetCheck##name##Impl<int, int>(v1, v2, names);              \
  }
DEFINE_RET_CHECK_OP_IMPL(_EQ, ==)
DEFINE_RET_CHECK_OP_IMPL(_NE, !=)
DEFINE_RET_CHECK_OP_IMPL(_LE, <=)
DEFINE_RET_CHECK_OP_IMPL(_LT, < )
DEFINE_RET_CHECK_OP_IMPL(_GE, >=)
DEFINE_RET_CHECK_OP_IMPL(_GT, > )
#undef DEFINE_RET_CHECK_OP_IMPL

#if defined(STATIC_ANALYSIS)
// Only for static analysis tool to know that it is equivalent to comparison.
#define RET_CHECK_OP(name, op, val1, val2) RET_CHECK((val1) op (val2))
#elif !defined(NDEBUG)
// In debug mode, avoid constructing CheckOpStrings if possible,
// to reduce the overhead of RET_CHECK statements.
#define RET_CHECK_OP(name, op, val1, val2) \
  while (std::string* _result = \
         ::basis::status_macros::RetCheck##name##Impl(      \
              google::GetReferenceableValue(val1),         \
              google::GetReferenceableValue(val2),         \
              #val1 " " #op " " #val2))                               \
    return ::basis::status_macros::MakeErrorStream(FROM_HERE, \
                                                  ::basis::error::INTERNAL) \
        .with_log_stack_trace() \
        .add_ret_check_failure( \
             ::basis::status_macros::internal::ErrorDeleteStringHelper( \
                 _result).str->c_str())
#else
// In optimized mode, use CheckOpString to hint to compiler that
// the while condition is unlikely.
#define RET_CHECK_OP(name, op, val1, val2) \
  while (CheckOpString _result = \
         ::basis::status_macros::RetCheck##name##Impl(      \
              google::GetReferenceableValue(val1),         \
              google::GetReferenceableValue(val2),         \
              #val1 " " #op " " #val2))                               \
    return ::basis::status_macros::MakeErrorStream(FROM_HERE, \
                                                  ::basis::error::INTERNAL) \
        .with_log_stack_trace() \
        .add_ret_check_failure( \
             ::basis::status_macros::internal::ErrorDeleteStringHelper( \
                 _result.str_).str->c_str())
#endif  // STATIC_ANALYSIS, !NDEBUG

// End of implementation code for RET_CHECK_EQ, RET_CHECK_NE, etc.
///////////////

// If the two values fail the comparison, this macro returns, from the current
// function, a ::basis::Status with code ::basis::error::INTERNAL.
// For example,
//   RET_CHECK_EQ(val1, val2) << message;
// is equivalent to
//   if(!(val1 == val2)) {
//     return MAKE_ERROR() << message;
//   }
// Note that the RET_CHECK macro includes some more information in the error
// and logs a stack trace.
//
// Intended to be used as a replacement for CHECK_* where crashes are
// unacceptable. The containing function must return a ::basis::Status.
#define RET_CHECK_EQ(val1, val2) RET_CHECK_OP(_EQ, ==, val1, val2)
#define RET_CHECK_NE(val1, val2) RET_CHECK_OP(_NE, !=, val1, val2)
#define RET_CHECK_LE(val1, val2) RET_CHECK_OP(_LE, <=, val1, val2)
#define RET_CHECK_LT(val1, val2) RET_CHECK_OP(_LT, < , val1, val2)
#define RET_CHECK_GE(val1, val2) RET_CHECK_OP(_GE, >=, val1, val2)
#define RET_CHECK_GT(val1, val2) RET_CHECK_OP(_GT, > , val1, val2)

// Unconditionally returns an error.  Use in place of RET_CHECK(false).
// Example:
//   if (a) {
//     ...
//   } else if (b) {
//     ...
//   } else {
//     RET_CHECK_FAIL() << "Failed to satisfy a or b";
//   }
#define RET_CHECK_FAIL() \
  LOG(ERROR) << "Return Error: " << " at "                          \
             << __FILE__ << ":" << __LINE__;                        \
  return ::basis::status_macros::MakeErrorStream(FROM_HERE,         \
                                                ::basis::error::INTERNAL) \
      .with_log_stack_trace() \
      .add_ret_check_fail_failure()

/// \note use only in tests (gtest)
/// EXPECT_* yields a nonfatal failure, allowing the function to continue running
#define EXPECT_OK(x) \
  EXPECT_TRUE(x.ok())

/// \note use only in tests (gtest)
/// ASSERT_* yields a fatal failure and returns from the current function
#define ASSERT_OK(x) \
  ASSERT_TRUE(x.ok())

/// \note use only in tests (gtest)
/// EXPECT_* yields a nonfatal failure, allowing the function to continue running
//
// USAGE
//
// using namespace basis::error;
// EXPECT_ERROR_CODE(OUT_OF_RANGE, tryAddMoney(a, b));
//
#define EXPECT_ERROR_CODE(code, x) \
  EXPECT_EQ(code, x.error_code())

/// \note use only in tests (gtest)
/// ASSERT_* yields a fatal failure and returns from the current function
//
// USAGE
//
// using namespace basis::error;
// ASSERT_ERROR_CODE(OUT_OF_RANGE, tryAddMoney(a, b));
//
#define ASSERT_ERROR_CODE(code, x) \
  ASSERT_EQ(code, x.error_code())

}  // namespace status_macros
}  // namespace basis
