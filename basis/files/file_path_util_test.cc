// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include "tests_common.h"

#include "basis/files/file_path_util.hpp"

#include <base/strings/strcat.h>
#include <base/strings/string_number_conversions.h>

#include <stdio.h>

#include <algorithm>

namespace basis {

TEST(FilePathUtilTest, EscapePath) {
  ASSERT_EQ(
    // Most of the character space we care about, un-escaped
    EscapePath(
      "\x02\n\x1d !\"#$%&'()*+,-./0123456789:;"
      "<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "[\\]^_`abcdefghijklmnopqrstuvwxyz"
      "{|}~\x7f\x80\xff"),
    // Escaped
    "%02%0A%1D%20!%22%23$%25&'()*+,-./0123456789%3A;"
    "%3C=%3E%3F@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz"
    "%7B%7C%7D~%7F%80%FF");
}

} // namespace basis
