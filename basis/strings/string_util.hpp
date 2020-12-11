#pragma once

#include <string>

#include <base/logging.h>
#include <base/macros.h>
#include <base/location.h>
#include <base/rvalue_cast.h>

namespace basis {

template<std::size_t N>
bool IsNullTerminated(const char(&str)[N]) {
  for (size_t i = 0; i < N; ++i) {
    if (str[i] == '\0') {
      return true;
    }
  }
  return false;
}

// Elide the given string `str` with '...' in the middle if the length
// exceeds `str` width.
//
// EXAMPLE
//
// ElideMiddle("01234567890123456789", 19) == "01234567...23456789"
// ElideMiddle("01234567890123456789", 10) == "012...789"
//
std::string ElideMiddle(const std::string& str, size_t width);

// Removes all Ansi escape codes (http://www.termsys.demon.co.uk/vtansi.htm).
//
// EXAMPLE
//
// StripAnsiEscapeCodes("foo\33[") == "foo"
//
std::string StripAnsiEscapeCodes(const std::string& in);

} // namespace basis
