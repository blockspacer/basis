// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


#include "basis/status/status.hpp"

#include <stdio.h>

#include <algorithm>

#include "absl/strings/str_cat.h"
#include "stratum/glue/init_google.h"
#include "stratum/glue/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  InitGoogle(argv[0], &argc, &argv, true);
#ifdef BENCHMARK
  RunSpecifiedBenchmarks();
#endif  // BENCHMARK
  return RUN_ALL_TESTS();
}

class MyErrorSpace : public ::basis::ErrorSpace {
 public:
  explicit MyErrorSpace(const char* name) : ::basis::ErrorSpace(name) {}
  std::string String(int code) const {
    return absl::StrCat("error(", code, "%d)");
  }

  ::::basis::error::Code CanonicalCode(const ::::basis::Status& status) const {
    switch (status.error_code()) {
      case 60:
        return ::::basis::error::PERMISSION_DENIED;
      default:
        return ::::basis::error::UNKNOWN;
    }
  }
};
static MyErrorSpace my_error_space("myerrors");
static MyErrorSpace my_error_space2("myerrors2");

// Typed null pointer for tests below
static const ::basis::ErrorSpace* kNullSpace =
    reinterpret_cast<::basis::ErrorSpace*>(NULL);

static const ::basis::ErrorSpace* OkSpace() {
  return ::::basis::Status::OK.error_space();
}

static const int CanonicalCode(const ::::basis::Status& s) {
  return s.ToCanonical().error_code();
}

// Check that s has the specified fields.
// An empty payload means the s must not contain a payload.
static void CheckStatus(const ::::basis::Status& s, const ::basis::ErrorSpace* space,
                        int code, const std::string& message) {
  EXPECT_EQ(code, s.error_code()) << s;
  EXPECT_EQ(space, s.error_space()) << s;
  EXPECT_EQ(message, s.error_message()) << s;

  if (code == 0) {
    EXPECT_TRUE(s.ok()) << s;
    EXPECT_EQ(0, CanonicalCode(s));
    CHECK_EQ("OK", s.ToString()) << s;
  } else {
    EXPECT_TRUE(!s.ok()) << s;
    EXPECT_EQ(::::basis::error::UNKNOWN, CanonicalCode(s));
    EXPECT_THAT(s.ToString(), testing::HasSubstr(space->SpaceName()));
    EXPECT_THAT(s.ToString(), testing::HasSubstr(space->String(code)));
    EXPECT_THAT(s.ToString(), testing::HasSubstr(message));
  }
}

TEST(ErrorSpace, SpaceName) {
  ASSERT_EQ(std::string("generic"),
            ::::basis::Status::OK.error_space()->SpaceName());
  ASSERT_EQ(std::string("myerrors"), my_error_space.SpaceName());
}

TEST(ErrorSpace, FindKnown) {
  ASSERT_EQ(&my_error_space, ::basis::ErrorSpace::Find("myerrors"));
  ASSERT_EQ(&my_error_space2, ::basis::ErrorSpace::Find("myerrors2"));
}

TEST(ErrorSpace, FindGeneric) {
  ASSERT_NE(kNullSpace, ::basis::ErrorSpace::Find("generic"));
}

TEST(ErrorSpace, FindUnknown) {
  ASSERT_EQ(kNullSpace, ::basis::ErrorSpace::Find("nonexistent_error_space"));
}

TEST(ErrorSpace, FindDestroyed) {
  { MyErrorSpace temp_space("temporary_name"); }
  ASSERT_EQ(kNullSpace, ::basis::ErrorSpace::Find("temporary_name"));
}

TEST(ErrorSpace, GenericCodeNames) {
  const ::basis::ErrorSpace* e = ::::basis::Status::CANCELLED.error_space();
  EXPECT_EQ("OK", e->String(::::basis::error::OK));
  EXPECT_EQ("cancelled", e->String(::::basis::error::CANCELLED));
  EXPECT_EQ("unknown", e->String(::::basis::error::UNKNOWN));
  EXPECT_EQ("aborted", e->String(::::basis::error::ABORTED));
  EXPECT_EQ("1000", e->String(1000));  // Out of range
}

TEST(Status, Empty) {
  ::::basis::Status status;
  CheckStatus(status, OkSpace(), 0, "");
}

TEST(Status, OK) { CheckStatus(::::basis::Status::OK, OkSpace(), 0, ""); }

TEST(Status, GenericCodes) {
  EXPECT_EQ(static_cast<int>(::::basis::error::OK),
            static_cast<int>(::::basis::Status::OK_CODE));
  EXPECT_EQ(static_cast<int>(::::basis::error::CANCELLED),
            static_cast<int>(::::basis::Status::CANCELLED_CODE));
  EXPECT_EQ(static_cast<int>(::::basis::error::UNKNOWN),
            static_cast<int>(::::basis::Status::UNKNOWN_CODE));
}

TEST(Status, ConstructorZero) {
  ::::basis::Status status(&my_error_space, 0, "msg");
  CheckStatus(status, OkSpace(), 0, "");
}

TEST(Status, CheckOK) {
  ::::basis::Status status;
  CHECK_OK(status);
  CHECK_OK(status) << "Failed";
  DCHECK_OK(status) << "Failed";
}

TEST(DeathStatus, CheckOK) {
  ::::basis::Status status;
  status =
      ::::basis::Status(::::basis::Status::canonical_space(),
                     ::::basis::Status::CANCELLED_CODE, "Operation Cancelled");
  ASSERT_DEATH(CHECK_OK(status), "Operation Cancelled");
}

TEST(Status, SetErrorZero) {
  ::::basis::Status status(&my_error_space, 2, "message");
  status = ::::basis::Status(&my_error_space, 0, "msg");
  CheckStatus(status, OkSpace(), 0, "");
}

TEST(Status, Cancelled) {
  ASSERT_THAT(::::basis::Status::CANCELLED.ToString(),
              testing::HasSubstr("cancel"));
}

TEST(Status, Filled) {
  ::::basis::Status status(&my_error_space, 2, "message");
  CheckStatus(status, &my_error_space, 2, "message");
}

TEST(Status, FilledNegative) {
  ::::basis::Status status(&my_error_space, -2, "message");
  CheckStatus(status, &my_error_space, -2, "message");
}

TEST(Status, Set) {
  ::::basis::Status status;
  status = ::::basis::Status(&my_error_space, 2, "message");
  CheckStatus(status, &my_error_space, 2, "message");
}

TEST(Status, SetOverlappingMessage) {
  ::::basis::Status status;
  status = ::::basis::Status(&my_error_space, 2, "message");
  CheckStatus(status, &my_error_space, 2, "message");

  std::string old_message = status.error_message();
  status = ::::basis::Status(&my_error_space, 2, old_message);
  CheckStatus(status, &my_error_space, 2, "message");

  std::string full_message = status.error_message();
  std::string part_message = full_message.substr(1, 3);
  EXPECT_EQ(part_message, "ess");
  status = ::::basis::Status(&my_error_space, 2, part_message);
  CheckStatus(status, &my_error_space, 2, "ess");
}

TEST(Status, Clear) {
  ::::basis::Status status(&my_error_space, 2, "message");
  status.Clear();
  CheckStatus(status, OkSpace(), 0, "");
}

TEST(Status, Copy) {
  ::::basis::Status a(&my_error_space, 2, "message");
  ::::basis::Status b(a);
  ASSERT_EQ(a.ToString(), b.ToString());
}

TEST(Status, Assign) {
  ::::basis::Status a(&my_error_space, 2, "message");
  ::::basis::Status b;
  b = a;
  ASSERT_EQ(a.ToString(), b.ToString());
}

TEST(Status, Update) {
  ::::basis::Status s;
  s.Update(::::basis::Status::OK);
  ASSERT_TRUE(s.ok());
  ::::basis::Status a(&my_error_space, 2, "message");
  s.Update(a);
  ASSERT_EQ(s.ToString(), a.ToString());
  ::::basis::Status b(&my_error_space, 17, "other message");
  s.Update(b);
  ASSERT_EQ(s.ToString(), a.ToString());
  s.Update(::::basis::Status::OK);
  ASSERT_EQ(s.ToString(), a.ToString());
  ASSERT_FALSE(s.ok());
}

TEST(Status, Swap) {
  ::::basis::Status a(&my_error_space, 2, "message");
  ::::basis::Status b = a;
  ::::basis::Status c;
  c.Swap(&a);
  ASSERT_EQ(c.ToString(), b.ToString());
  ASSERT_EQ(a.ToString(), ::::basis::Status::OK.ToString());
}

TEST(Status, UnknownCode) {
  ::::basis::Status status(&my_error_space, 10, "message");
  ASSERT_TRUE(!status.ok());
  ASSERT_EQ(10, status.error_code());
  ASSERT_EQ(::::basis::error::UNKNOWN, CanonicalCode(status));
  ASSERT_EQ(std::string("message"), status.error_message());
  ASSERT_EQ(status.error_space(), &my_error_space);
  ASSERT_THAT(status.ToString(),
              testing::MatchesRegex("myerrors.*10.*message"));
}

TEST(Status, MatchOK) {
  ASSERT_TRUE(::::basis::Status().Matches(::::basis::Status::OK));
}

TEST(Status, MatchSame) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 1, "message");
  ASSERT_TRUE(a.Matches(b));
}

TEST(Status, MatchCopy) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = a;
  ASSERT_TRUE(a.Matches(b));
}

TEST(Status, MatchDifferentCode) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 2, "message");
  ASSERT_TRUE(!a.Matches(b));
}

TEST(Status, MatchDifferentSpace) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space2, 1, "message");
  ASSERT_TRUE(!a.Matches(b));
}

TEST(Status, MatchDifferentMessage) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 1, "another");
  ASSERT_TRUE(a.Matches(b));
}

TEST(Status, EqualsOK) { ASSERT_EQ(::::basis::Status::OK, ::::basis::Status()); }

TEST(Status, EqualsSame) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 1, "message");
  ASSERT_EQ(a, b);
}

TEST(Status, EqualsCopy) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = a;
  ASSERT_EQ(a, b);
}

TEST(Status, EqualsDifferentCode) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 2, "message");
  ASSERT_NE(a, b);
}

TEST(Status, EqualsDifferentSpace) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space2, 1, "message");
  ASSERT_NE(a, b);
}

TEST(Status, EqualsDifferentMessage) {
  const ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "message");
  const ::::basis::Status b = ::::basis::Status(&my_error_space, 1, "another");
  ASSERT_NE(a, b);
}

TEST(Status, EqualsCanonicalCodeSame) {
  ::::basis::Status a = ::::basis::Status(&my_error_space, 1234, "message");
  ::::basis::Status b = ::::basis::Status(&my_error_space, 1234, "message");
  ASSERT_EQ(a, b);
  a.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  b.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  ASSERT_EQ(a, b);
}

TEST(Status, EqualsCanonicalCodeMismatch) {
  ::::basis::Status a = ::::basis::Status(&my_error_space, 1234, "message");
  ::::basis::Status b = ::::basis::Status(&my_error_space, 1234, "message");
  ASSERT_EQ(a, b);
  a.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  b.SetCanonicalCode(::::basis::error::UNAVAILABLE);
  ASSERT_NE(a, b);
}

TEST(Status, StripMessage) {
  ::::basis::Status a = ::::basis::Status(&my_error_space, 1, "");
  ::::basis::Status b = ::::basis::Status(&my_error_space, 1, "x");
  ASSERT_EQ(a, b.StripMessage());
}

static void SanityCheck(const ::::basis::Status& s, const ::basis::ErrorSpace* space,
                        int code, const std::string& msg) {
  EXPECT_EQ(code, s.error_code());
  EXPECT_EQ(space, s.error_space());

  ::::basis::Status copy(s);
  EXPECT_EQ(s, copy);

  ::::basis::Status other(::::basis::error::DEADLINE_EXCEEDED, "_sanity_check_");
  EXPECT_NE(other, s);

  ::::basis::Status updated;
  updated.Update(s);
  EXPECT_EQ(s, updated);

  // Matches / StripMessage
  ::::basis::Status with_msg(space, code, "_sanity_check_");
  EXPECT_TRUE(s.Matches(with_msg));
  EXPECT_EQ(s, with_msg.StripMessage());
  if (!s.ok()) {
    EXPECT_FALSE(s == with_msg);
  }

  // SetError
  ::::basis::Status err;
  err = ::::basis::Status(space, code, msg);
  EXPECT_EQ(s, err);
}

TEST(Status, Globals) {
  const ::basis::ErrorSpace* space = ::::basis::Status::canonical_space();
  SanityCheck(::::basis::Status::OK, space, ::::basis::Status::OK_CODE, "");
  SanityCheck(::::basis::Status::CANCELLED, space, ::::basis::Status::CANCELLED_CODE,
              "");
  SanityCheck(::::basis::Status::UNKNOWN, space, ::::basis::Status::UNKNOWN_CODE, "");
}

TEST(Canonical, WrongSpace) {
  ::::basis::Status status(&my_error_space, 1, "message");
  const ::basis::ErrorSpace* space = ::::basis::Status::canonical_space();
  EXPECT_EQ(::::basis::error::UNKNOWN, space->CanonicalCode(status));
}

TEST(Canonical, CustomMapping) {
  ::::basis::Status s(&my_error_space, 60, "message");
  EXPECT_EQ(::::basis::error::PERMISSION_DENIED, CanonicalCode(s));
}

static void VerifyCanonical(const ::::basis::Status& s,
                            ::::basis::error::Code match_code,
                            ::::basis::error::Code nomatch_code) {
  EXPECT_EQ(match_code, s.CanonicalCode());
  EXPECT_TRUE(s.Matches(match_code)) << match_code;
  EXPECT_FALSE(s.Matches(nomatch_code)) << nomatch_code;
}

TEST(Canonical, CanonicalCode) {
  ::::basis::Status ok = ::::basis::Status::OK;
  ::::basis::Status cancel = ::::basis::Status::CANCELLED;
  ::::basis::Status perm(&my_error_space, 60, "message");
  ::::basis::Status other(&my_error_space, 10, "message");
  VerifyCanonical(ok, ::::basis::error::OK, ::::basis::error::UNKNOWN);
  VerifyCanonical(cancel, ::::basis::error::CANCELLED, ::::basis::error::UNKNOWN);
  VerifyCanonical(perm, ::::basis::error::PERMISSION_DENIED,
                  ::::basis::error::UNKNOWN);
  VerifyCanonical(other, ::::basis::error::UNKNOWN,
                  ::::basis::error::PERMISSION_DENIED);

  // Check handling of a canonical code not known in this address space.
  perm.SetCanonicalCode(static_cast<int>(::::basis::error::Code_MAX) + 1);
  VerifyCanonical(perm, ::::basis::error::UNKNOWN,
                  ::::basis::error::PERMISSION_DENIED);
}

TEST(Canonical, SetCanonicalCode) {
  ::::basis::Status s(&my_error_space, 1234, "message");
  s.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  EXPECT_EQ(1234, s.error_code());
  EXPECT_EQ(::::basis::error::RESOURCE_EXHAUSTED, CanonicalCode(s));
}

TEST(Canonical, SetCanonicalCodeIgnoredOnOkStatus) {
  ::::basis::Status s(&my_error_space, 0, "message");
  s.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(::::basis::error::OK, CanonicalCode(s));
}

TEST(Canonical, SetCanonicalCodeIgnoredOnCanonicalSpace) {
  ::::basis::Status s(::::basis::error::DEADLINE_EXCEEDED, "message");
  s.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  EXPECT_EQ(::::basis::error::DEADLINE_EXCEEDED, s.error_code());
  EXPECT_EQ(::::basis::error::DEADLINE_EXCEEDED, CanonicalCode(s));
}

TEST(Canonical, SetCanonicalCodeOnSharedStatus) {
  const ::::basis::Status x(&my_error_space, 1234, "message");

  // Set canonical code on a copy.
  ::::basis::Status y = x;
  y.SetCanonicalCode(::::basis::error::RESOURCE_EXHAUSTED);
  EXPECT_NE(x, y);
  EXPECT_EQ(x.error_space(), y.error_space());
  EXPECT_EQ(x.error_code(), y.error_code());
  EXPECT_EQ(x.error_message(), y.error_message());
  EXPECT_EQ(::::basis::error::UNKNOWN, CanonicalCode(x));
  EXPECT_EQ(::::basis::error::RESOURCE_EXHAUSTED, CanonicalCode(y));

  // Yet another copy, with a different code set.
  ::::basis::Status z = y;
  z.SetCanonicalCode(::::basis::error::DEADLINE_EXCEEDED);
  EXPECT_NE(y, z);
  EXPECT_EQ(x.error_space(), z.error_space());
  EXPECT_EQ(x.error_code(), z.error_code());
  EXPECT_EQ(x.error_message(), z.error_message());
  EXPECT_EQ(::::basis::error::RESOURCE_EXHAUSTED, CanonicalCode(y));
  EXPECT_EQ(::::basis::error::DEADLINE_EXCEEDED, CanonicalCode(z));
}

#ifdef BENCHMARK
static void BM_StatusCreateDestroy(int iters) {
  int count = 0;
  ::::basis::Status dummy = ::::basis::Status::CANCELLED;
  for (int i = 0; i < iters; i++) {
    ::::basis::Status s;
    if (i == 17) {
      s = dummy;
    }
    if (!s.ok()) {
      count++;
    }
  }
  if (count == -1) {
    fprintf(stderr, "Dummy use");
  }
}
BENCHMARK(BM_StatusCreateDestroy);

static void BM_StatusCopy(int iters) {
  ::::basis::Status dummy[2];
  dummy[1] = ::::basis::Status::CANCELLED;
  int count = 0;
  for (int i = 0; i < iters; i++) {
    ::::basis::Status s = dummy[i == 17];
    if (!s.ok()) {
      count++;
    }
  }
  if (count == -1) {
    fprintf(stderr, "Dummy use");
  }
}
BENCHMARK(BM_StatusCopy);
#endif  // BENCHMARK