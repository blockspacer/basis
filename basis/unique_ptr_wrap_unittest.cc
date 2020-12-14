#include "testsCommon.h"

#if !defined(USE_GTEST_TEST)
#warning "use USE_GTEST_TEST"
// default
#define USE_GTEST_TEST 1
#endif // !defined(USE_GTEST_TEST)

#include "basis/unique_ptr_wrap.hpp"

#include <cstddef>

namespace base {

namespace {

class DeleteCounter {
 public:
  DeleteCounter() { ++count_; }
  ~DeleteCounter() { --count_; }

  static size_t count() { return count_; }

 private:
  static size_t count_;
};

size_t DeleteCounter::count_ = 0;

}  // namespace

TEST(PtrUtilTest, WrapUniqueNotArray) {
  EXPECT_EQ(0u, DeleteCounter::count());
  DeleteCounter* counter = new DeleteCounter;
  EXPECT_EQ(1u, DeleteCounter::count());
  std::unique_ptr<DeleteCounter> owned_counter = WrapUniqueNotArray(counter);
  EXPECT_EQ(1u, DeleteCounter::count());
  owned_counter.reset();
  EXPECT_EQ(0u, DeleteCounter::count());
}

}  // namespace base
