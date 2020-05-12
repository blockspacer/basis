#include "basis/i18n.hpp"

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

#if !UCONFIG_NO_FORMATTING

namespace i18n {

I18n::I18n(const char* pref_locale)
{
  DETACH_FROM_SEQUENCE(sequence_checker_);

  if(pref_locale == nullptr) {
    base::i18n::SetICUDefaultLocale(uloc_getDefault());
    // Convert ICU locale to chrome ("en_US" to "en-US", etc.).
    std::string canonicalLocale
      = base::i18n::GetCanonicalLocale(uloc_getDefault());
    VLOG(9)
      << "SetICUDefaultLocale: "
      << uloc_getDefault()
      << " as CanonicalLocale: "
      << canonicalLocale;
  }

  // sanity check
  {
    const base::string16 pattern = base::ASCIIToUTF16(
        "{1, plural, "
        "=1 {The cert for {0} expired yesterday.}"
        "=7 {The cert for {0} expired a week ago.}"
        "other {The cert for {0} expired # days ago.}}");
    std::string result
      = base::UTF16ToASCII(
          base::i18n::MessageFormatter::FormatWithNumberedArgs(
        pattern, "example.com", 1));
    DCHECK_EQ("The cert for example.com expired yesterday.", result);
    result = base::UTF16ToASCII(
      base::i18n::MessageFormatter::FormatWithNumberedArgs(
        pattern, "example.com", 7));
    DCHECK_EQ("The cert for example.com expired a week ago.", result);
    result = base::UTF16ToASCII(
      base::i18n::MessageFormatter::FormatWithNumberedArgs(
        pattern, "example.com", 15));
    DCHECK_EQ("The cert for example.com expired 15 days ago.", result);
  }
}

I18n::~I18n()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

#else // UCONFIG_NO_FORMATTING

#error "unable to init i18n: icu built with UCONFIG_NO_FORMATTING"

#endif // UCONFIG_NO_FORMATTING

} // namespace i18n
