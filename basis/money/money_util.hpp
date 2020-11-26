#pragma once

#include <base/logging.h>

#include <basis/money/money.hpp>
#include <basis/status/statusor.hpp>
#include <basis/status/status_macros.hpp>

#define CHECK_MONEY_VALID(money) \
  do { \
    CHECK(::basis::validateMoney(money).ok()); \
    CHECK_MONEY_NANOS(money); \
  } while (0)

#define CHECK_MONEY_NANOS(money) \
  do { \
    CHECK(money.units > 0 ? money.nanos >= 0 : true) \
      << "If `units` is positive, `nanos` must be positive or zero."; \
    CHECK(money.units < 0 ? money.nanos <= 0 : true) \
      << "If `units` is negative, `nanos` must be negative or zero."; \
  } while (0)

#define CHECK_MONEY_NOT_NEGATIVE(money) \
  do { \
    CHECK_GE(money.units, 0); \
    CHECK_MONEY_NANOS(money); \
  } while (0)

#define CHECK_MONEY_NOT_POSITIVE(money) \
  do { \
    CHECK_LE(money.units, 0); \
    CHECK_MONEY_NANOS(money); \
  } while (0)

namespace basis {

// Returns OK if the given money is a valid value. The possible validation
// errors include invalid currency_code format, nanos out of range, and
// the signs of units and nanos disagree. In all error cases the error
// code is INVALID_ARGUMENT, with an error message.
MUST_USE_RETURN_VALUE
Status validateMoney(
  const Money& money);

// Returns 1 if the given money has a positive amount, 0 if money has zero
// amount, and -1 if money has a negative amount.
// The given money must be valid (see Validate) or the result may be wrong.
//
// EXAMPLE
//
// Sign of $-2.75 is -1.
// Sign of $8.3 is 1.
// Sign of $0.0 is 0.
MUST_USE_RETURN_VALUE
int getAmountSign(const Money& money);

// Adds a and b together into sum. The caller owns the lifetime of sum. Both
// a and b must be valid money values (see ValidateMoney), otherwise sum may
// contain invalid value.
// Returns OK if successful. There are two possible errors:
// (1) If the currency_code of a and b are different, sum is cleared and
// INVALID_ARGUMENT is returned.
// (2) If arithmetic overflow occurs during the additions, sum is set to the
// maximum positive or minimum negative amount depending on the direction of
// the overflow, and OUT_OF_RANGE is returned.
MUST_USE_RETURN_VALUE
StatusOr<Money> tryAddMoney(const Money& a
  , const Money& b
  , bool fail_on_overflow = true);

// Returns the sum of a and b. Both a and b must be valid money values (see
// ValidateMoney), otherwise the result may contain invalid value. The
// caller must ensure a and b have the same currency_code, otherwise it's a
// fatal error.
//
MUST_USE_RETURN_VALUE
StatusOr<Money> saturatedAddMoney(const Money& a
  , const Money& b);

} // namespace basis
