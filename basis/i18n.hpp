#pragma once

#include <memory>

#include <base/i18n/rtl.h>
#include <base/i18n/icu_util.h>
#include <base/i18n/message_formatter.h>
#include <base/strings/string_piece.h>
#include <base/strings/string_util.h>
#include <base/strings/utf_string_conversions.h>
#include <base/time/time.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/sequenced_task_runner.h>

#include <third_party/icu/source/common/unicode/unistr.h>
#include <third_party/icu/source/i18n/unicode/datefmt.h>
#include <third_party/icu/source/i18n/unicode/msgfmt.h>
#include <third_party/icu/source/common/unicode/uloc.h>

namespace i18n {

class I18n {
public:
  I18n(const char* locale = nullptr);

  ~I18n();

private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(I18n);
};

} // namespace i18n
