// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include "basis/status/status.hpp" // IWYU pragma: associated

#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include <base/logging.h>
#include <base/strings/strcat.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <base/synchronization/lock.h>
#include <base/lazy_instance.h>
#include <base/strings/substitute.h>

namespace basis {

// Global registry
typedef std::unordered_map<std::string, ErrorSpace*, std::hash<std::string> >
    ErrorSpaceTable;
static ErrorSpaceTable* error_space_table;

base::LazyInstance<::base::Lock>::Leaky registry_lock_ =
  LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<::base::Lock>::Leaky init_lock_ =
  LAZY_INSTANCE_INITIALIZER;

// Convert canonical code to a value known to this binary.
static inline error::Code MapToLocalCode(int c) {
  return error::Code_IsValid(c) ? static_cast<error::Code>(c) : error::UNKNOWN;
}

namespace error {

inline std::string CodeEnumToString(error::Code code) {
  switch (code) {
    case OK:
      return "OK";
    case CANCELLED:
      return "CANCELLED";
    case UNKNOWN:
      return "UNKNOWN";
    case INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case DEADLINE_EXCEEDED:
      return "DEADLINE_EXCEEDED";
    case NOT_FOUND:
      return "NOT_FOUND";
    case ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case UNAUTHENTICATED:
      return "UNAUTHENTICATED";
    case RESOURCE_EXHAUSTED:
      return "RESOURCE_EXHAUSTED";
    case FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case ABORTED:
      return "ABORTED";
    case OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case INTERNAL:
      return "INTERNAL";
    case UNAVAILABLE:
      return "UNAVAILABLE";
    case DATA_LOSS:
      return "DATA_LOSS";
    case DO_NOT_USE_RESERVED_FOR_FUTURE_EXPANSION_USE_DEFAULT_IN_SWITCH_INSTEAD_:  // NOLINT
      // We are not adding a default clause here, to explicitly make clang
      // detect the missing codes. This conversion method must stay in sync
      // with codes.proto.
      return "UNKNOWN";
  }

  // No default clause, clang will abort if a code is missing from
  // above switch.
  return "UNKNOWN";
}

}  // namespace error.

// Special space for the OK error.
class GenericErrorSpace : public ErrorSpace {
 public:
  GenericErrorSpace() : ErrorSpace("generic") {}

  virtual std::string String(int code) const {
    std::string status;
    if (code == 0) {
      status = "OK";
    } else if (error::Code_IsValid(code)) {
      // Lower-case the protocol-compiler assigned name for compatibility
      // with old behavior.
      status = ::base::ToLowerASCII(
          error::CodeEnumToString(static_cast<error::Code>(code)));
    } else {
      status = base::Substitute("$1", code);
    }
    return status;
  }

  virtual error::Code CanonicalCode(const ::basis::Status& status) const {
    if (status.error_space() == Status::canonical_space()) {
      return MapToLocalCode(status.error_code());
    }
    return error::UNKNOWN;
  }
};

static bool initialized = false;
static const ErrorSpace* generic_space = nullptr;
static const std::string* empty_string;

static void InitModule() {
  ::base::AutoLock initLock(init_lock_.Get());
  if (initialized) return;
  initialized = true;
  generic_space = new GenericErrorSpace;
  empty_string = new std::string;
}

const ErrorSpace* Status::canonical_space() {
  {
    InitModule();
  }
  return generic_space;
}

const std::string* Status::EmptyString() {
  {
    InitModule();
  }
  return empty_string;
}

// Representation for global objects.
struct Status::Pod {
  // Structured exactly like ::basis::Status, but has no constructor so
  // it can be statically initialized
  Rep* rep_;
};

Status::Rep Status::global_reps[3] = {
  {ATOMIC_VAR_INIT(kGlobalRef), OK_CODE, 0, nullptr, nullptr, FROM_HERE}
};

const Status::Pod Status::globals[3] = {{&Status::global_reps[0]}};

void Status::UnrefSlow(Rep* r) {
  DCHECK(r->ref != kGlobalRef);
  // Fast path: if ref==1, there is no need for a RefCountDec (since
  // this is the only reference and therefore no other thread is
  // allowed to be mucking with r).
  if (r->ref == 1 || --r->ref == 0) { /// \todo magic const 1???
    delete r->message_ptr;
    delete r;
  }
}

Status::Rep* Status::NewRep(const ::base::Location& location
  , const ErrorSpace* space
  , int code
  , const std::string& msg
  , int canonical_code)
{
  DCHECK(space != nullptr);
  Rep* rep = new Rep;
  rep->ref = 1; /// \todo magic const 1???
  rep->message_ptr = nullptr;
  rep->location = location;
  ResetRep(rep, location, space, code, msg, canonical_code);
  return rep;
}

void Status::ResetRep(Rep* rep
  , const ::base::Location& location
  , const ErrorSpace* space
  , int code
  , const std::string& msg
  , int canonical_code)
{
  DCHECK(rep != nullptr);
  DCHECK_EQ(rep->ref, 1); /// \todo magic const 1???
  DCHECK(space != canonical_space() || canonical_code == 0);
  rep->code = code;
  rep->space_ptr = space;
  rep->canonical_code = canonical_code;
  rep->location = location;
  if (rep->message_ptr == nullptr) {
    rep->message_ptr = new std::string(msg.data(), msg.size());
  } else if (msg != *rep->message_ptr) {
    // msg is not identical to current rep->message.
    std::string copy = msg;
    swap(*rep->message_ptr, copy);
  }
}

Status::Status(const ::base::Location& location
  , error::Code code
  , const std::string& msg)
{
  rep_ = NewRep(location, canonical_space(), code, msg, ::basis::error::OK);
}

Status::Status(const ::base::Location& location
  , const ErrorSpace* space
  , int code
  , const std::string& msg)
{
  DCHECK(space != nullptr);
  rep_ = NewRep(location, space, code, msg, ::basis::error::OK);
}

int Status::RawCanonicalCode() const {
  if (rep_->canonical_code > 0) {
    return rep_->canonical_code;
  } else if (error_space() == Status::canonical_space()) {
    return error_code();
  } else {
    return error_space()->CanonicalCode(*this);
  }
}

error::Code Status::CanonicalCode() const {
  return MapToLocalCode(RawCanonicalCode());
}

void Status::SetCanonicalCode(int canonical_code) {
  if (error_space() != Status::canonical_space()) {
    PrepareToModify();
    rep_->canonical_code = canonical_code;
  }
}

Status Status::ToCanonical() const {
  int code = RawCanonicalCode();
  return Status(location(), canonical_space(), code, error_message());
}

void Status::Clear() {
  Unref(rep_);
  rep_ = &global_reps[0];
}

void Status::SetError(const ::base::Location& location
  , const ErrorSpace* space
  , int code
  , const std::string& msg)
{
  InternalSet(location, space, code, msg, ::basis::error::OK);
}

void Status::PrepareToModify() {
  DCHECK(!ok());
  if (rep_->ref != 1) { /// \todo magic const 1???
    Rep* old_rep = rep_;
    rep_ = NewRep(location(), error_space(), error_code(), error_message(),
                  old_rep->canonical_code);
    Unref(old_rep);
  }
}

void Status::InternalSet(const ::base::Location& location
  , const ErrorSpace* space
  , int code
  , const std::string& msg
  , int canonical_code)
{
  DCHECK(space != nullptr);
  if (code == 0) { /// \todo magic const 0???
    // Construct an OK status
    Clear();
  } else if (rep_->ref == 1) { /// \todo magic const 1???
    // Update in place
    ResetRep(rep_, location, space, code, msg, canonical_code);
  } else {
    // If we are doing an update, then msg may point into rep_.
    // Wait to Unref rep_ *after* we copy these into the new rep_,
    // so that it will stay alive and unmodified while we're working.
    Rep* old_rep = rep_;
    rep_ = NewRep(location, space, code, msg, canonical_code);
    Unref(old_rep);
  }
}

bool Status::EqualsSlow(const ::basis::Status& a, const ::basis::Status& b) {
  /// \note ignore location() in comparison
  if ((a.error_code() == b.error_code()) &&
      (a.error_space() == b.error_space()) &&
      (a.error_message() == b.error_message()) &&
      (a.RawCanonicalCode() == b.RawCanonicalCode())) {
    return true;
  }
  return false;
}

std::string Status::ToString() const {
  std::string status;
  const int code = error_code();
  if (code == 0) {
    status = base::Substitute(
      "OK ($1)"
      , location());
  } else {
    const ErrorSpace* const space = error_space();
    DCHECK_PTR(space);
    status = base::Substitute(
      "$1 $2 $3 ($4)"
      , space->SpaceName()
      , space->String(code)
      , error_message()
      , location());
  }

  return status;
}

std::ostream& operator<<(std::ostream& os, const Status& x) {
  os << x.ToString();
  return os;
}

void Status::CheckMatches(const Status& x) const {
  CHECK(Matches(x)) << ToString() << " does not match " << x.ToString();
}

void Status::IgnoreError() const {
  // no-op
}

Status Status::StripMessage() const {
  return Status(location(), error_space(), error_code(), std::string());
}

ErrorSpace::ErrorSpace(const char* name) : name_(name) {
  ::base::AutoLock l(registry_lock_.Get());
  if (error_space_table == nullptr) {
    error_space_table = new ErrorSpaceTable;
  }
  (*error_space_table)[name_] = this;
}

ErrorSpace::~ErrorSpace() {
  ::base::AutoLock l(registry_lock_.Get());
  ErrorSpaceTable::iterator iter = error_space_table->find(name_);
  if (iter != error_space_table->end() && iter->second == this) {
    error_space_table->erase(iter);
  }
}

ErrorSpace* ErrorSpace::Find(const std::string& name) {
  {
    InitModule();
  }
  ::base::AutoLock l(registry_lock_.Get());
  if (error_space_table == nullptr) {
    return nullptr;
  } else {
    ErrorSpaceTable::const_iterator iter = error_space_table->find(name);
    if (iter == error_space_table->end()) {
      return nullptr;
    } else {
      return iter->second;
    }
  }
}

// Provide default implementations of abstract methods in case
// somehow somebody ends up invoking one of these methods during
// the subclass construction/destruction phase.
std::string ErrorSpace::String(int code) const {
  return base::Substitute("$1", code);
}

// Register canoncial error space.
// This forces InitModule to run.
static const ErrorSpace* dummy __attribute__((unused)) =
    ::basis::Status::canonical_space();

}  // namespace basis
