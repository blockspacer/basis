#pragma once

#include <base/files/file_path.h>

namespace icu_util {

extern const base::FilePath::CharType kIcuDataFileName[];

void initICUi18n(
  const base::FilePath::CharType icuFileName[] = kIcuDataFileName);

}  // namespace icu_util
