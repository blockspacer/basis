#include "tests_common.h"
#include "basis/test/test_macros.hpp"

#include "basis/strings/format.hpp"

#include <base/test/scoped_task_environment.h>
#include <base/logging.h>
#include <base/strings/utf_string_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/string16.h>
#include <base/macros.h>
#include <base/stl_util.h>

#include <iostream>

class Date
{
public:
    Date(int year, int month, int day) : _year(year), _month(month), _day(day) {}

    friend std::ostream& operator<<(std::ostream& os, const Date& date)
    {
      return os << date._year << '-' << date._month << '-' << date._day;
    }

private:
    int _year, _month, _day;
};

struct S {
  double value;
};

std::ostream& operator<<(std::ostream& os, S s) {
  return os << s.value;
}

#define SHOW(expression) LOG(INFO) << #expression << " = \"" << expression << "\"";

namespace basis {

class StringFormatDemo : public testing::Test {
 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(StringFormatDemo, Simple) {
  using namespace fmt::literals;

  EXPECT_EQ(basis::fmt("no arguments"), "no arguments");

#if 0
  // basis::fmt("{0}, {1}, {2}", -1, 0, 1) = "-1, 0, 1"
  SHOW(basis::fmt("{0}, {1}, {2}", -1, 0, 1));

  // basis::fmt("{0}, {1}, {2}", 'a', 'b', 'c') = "a, b, c"
  SHOW(basis::fmt("{0}, {1}, {2}", 'a', 'b', 'c'));

  // basis::fmt("{}, {}, {}", 'a', 'b', 'c') = "a, b, c"
  SHOW(basis::fmt("{}, {}, {}", 'a', 'b', 'c'));

  // basis::fmt("{2}, {1}, {0}", 'a', 'b', 'c') = "c, b, a"
  SHOW(basis::fmt("{2}, {1}, {0}", 'a', 'b', 'c'));

  // basis::fmt("{0}{1}{0}", "abra", "cad") = "abracadabra"
  SHOW(basis::fmt("{0}{1}{0}", "abra", "cad"));

  // basis::fmt("{:<30}", "left aligned") = "left aligned                  "
  SHOW(basis::fmt("{:<30}", "left aligned"));

  // basis::fmt("{:>30}", "right aligned") = "                 right aligned"
  SHOW(basis::fmt("{:>30}", "right aligned"));

  // basis::fmt("{:^30}", "centered") = "           centered           "
  SHOW(basis::fmt("{:^30}", "centered"));

  // basis::fmt("{:*^30}", "centered") = "***********centered***********"
  SHOW(basis::fmt("{:*^30}", "centered"));

  // basis::fmt("{:+f}; {:+f}", 3.14, -3.14) = "+3.140000; -3.140000"
  SHOW(basis::fmt("{:+f}; {:+f}", 3.14, -3.14));

  // basis::fmt("{: f}; {: f}", 3.14, -3.14) = " 3.140000; -3.140000"
  SHOW(basis::fmt("{: f}; {: f}", 3.14, -3.14));

  // basis::fmt("{:-f}; {:-f}", 3.14, -3.14) = "3.140000; -3.140000"
  SHOW(basis::fmt("{:-f}; {:-f}", 3.14, -3.14));

  // basis::fmt("int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42) = "int: 42;  hex: 2a;  oct: 52; bin: 101010"
  SHOW(basis::fmt("int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42));

  // basis::fmt("int: {0:d};  hex: {0:#x};  oct: {0:#o};  bin: {0:#b}", 42) = "int: 42;  hex: 0x2a;  oct: 052;  bin: 0b101010"
  SHOW(basis::fmt("int: {0:d};  hex: {0:#x};  oct: {0:#o};  bin: {0:#b}", 42));

  // basis::fmt("The date is {}", Date(2012, 12, 9)) = "The date is 2012-12-9"
  SHOW(basis::fmt("The date is {}", Date(2012, 12, 9)));

  // basis::fmt("Elapsed time: {s:.2f} seconds", "s"_a = 1.23) = "Elapsed time: 1.23 seconds"
  SHOW(basis::fmt("Elapsed time: {s:.2f} seconds", "s"_a = 1.23));

  // "The answer is {}"_format(42) = "The answer is 42"
  SHOW("The answer is {}"_format(42));

  // ??????????????????????
  // ?    Привет, мир!    ?
  // ??????????????????????
  SHOW(basis::fmt(u8"?{0:?^{2}}?\n"
                  "?{1: ^{2}}?\n"
                  "?{0:?^{2}}?\n", "", u8"Привет, мир!", 20));

  // 0,42
  SHOW(::fmt::format(std::locale("fr_FR.UTF-8"), "{}", S{0.42}));

  {
    // Привет мир! -1, 0, 1
    std::wstring hello_world = base::UTF8ToWide(u8"\u041F\u0440\u0438\u0432\u0435\u0442 {0}, {1}, {2}, {3}");
    SHOW(basis::fmt(hello_world, base::UTF8ToWide(u8"\u043C\u0438\u0440!"), -1, 0, 1));
  }
#endif
}

} // namespace basis
