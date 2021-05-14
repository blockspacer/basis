// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "build/build_config.h"

#include <locale>

int main(int argc, char** argv) {
  CHECK(setlocale(LC_ALL, "en_US.UTF-8") != nullptr)
      << "Failed to set locale: " << "en_US.UTF-8";

  // Various things break when you're using a locale where the decimal
  // separator isn't a period.
  CHECK(setlocale(LC_NUMERIC, "C") != nullptr)
      << "Failed to set locale: " << "LC_NUMERIC C";

  base::TestSuite test_suite(argc, argv);
  return base::LaunchUnitTests(
      argc, argv,
      base::BindOnce(&base::TestSuite::Run, base::Unretained(&test_suite)));
}
