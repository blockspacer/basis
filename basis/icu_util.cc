#include "basis/icu_util.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/i18n/icu_util.h>

namespace icu_util {

const base::FilePath::CharType kIcuDataFileName[]
  = FILE_PATH_LITERAL("./resources/icu/icudtl.dat");

void initICUi18n(
  const base::FilePath::CharType icuFileName[])
{
  base::FilePath dir_exe;
  if (!base::PathService::Get(base::DIR_EXE, &dir_exe)) {
    NOTREACHED();
  }
  if(!base::PathExists(dir_exe.Append(icuFileName))) {
    LOG(WARNING)
      << "unable to find icu i18n file: "
      << dir_exe.Append(icuFileName);
  } else {
    DVLOG(9)
      << "loading icu i18n file: "
      << dir_exe.Append(icuFileName);
  }
  bool icu_initialized
    = base::i18n::InitializeICUWithPath(
        dir_exe.Append(icuFileName));
  if(!icu_initialized) {
    LOG(WARNING)
      << "unable to initialize icu i18n file: "
      << dir_exe.Append(icuFileName);
  }
}

}  // namespace icu_util
