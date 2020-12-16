// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "basis/files/scoped_temp_file.hpp" // IWYU pragma: associated
#include "base/logging.h"
#include "base/files/file_util.h"
#include "base/files/file.h"

namespace basis {

ScopedTempFile::ScopedTempFile() {
  CHECK(base::CreateTemporaryFile(&path_));
}

ScopedTempFile::~ScopedTempFile() {
  if (FileExists()) {
    CHECK(base::DeleteFile(path_, /*recursive*/false));
  }
}

bool ScopedTempFile::FileExists() const {
  return base::PathExists(path_);
}

int ScopedTempFile::Write(const std::string& str) {
  CHECK(FileExists());
  return base::WriteFile(path_, str.c_str(), str.size());
}

std::string ScopedTempFile::Read() const {
  CHECK(FileExists());
  std::string result;
  CHECK(ReadFileToString(path_, &result));
  return result;
}

}  // namespace basis
