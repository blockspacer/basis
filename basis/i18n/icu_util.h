#pragma once

#include <base/files/file_path.h>

namespace basis {

extern const ::base::FilePath::CharType kIcuDataFileName[];

void initICUi18n(
  const ::base::FilePath::CharType icuFileName[] = kIcuDataFileName);

}  // namespace basis
