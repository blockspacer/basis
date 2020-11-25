#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>
#include <base/time/time.h>

#include <basis/checks_and_guard_annotations.hpp>

namespace base {


// Check that callback will be executed with delay <= `base::TimeDelta`.
// Check of delay time will be performed on each call.
//
/// \note Check will be performed NOT on callback 'body',
/// but on time delay before callback call.
//
// USAGE
//
// class TmpClass
// {
//  public:
//   void TestMe(const ::base::Location& location)
//   {
//     ::base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(2));
//   }
// };
// {
//   ::base::RepeatingCallback<void(const ::base::Location& location)> repCb
//     = ::base::bindCheckedRepeating(
//         DEBUG_BIND_CHECKS(
//           DELAY_TIME_LIMIT_CHECKER(base::TimeDelta::FromSeconds(3))
//         )
//         , &TmpClass::TestMe
//         , ::base::Unretained(&tmpClass)
//     );
//
//   repCb.Run(FROM_HERE); // delay check ok + elapsed 2 sec in `&TmpClass::TestMe`
//
//   ::base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(2));
//
//   repCb.Run(FROM_HERE); // delay check fail, elapsed 4 sec totally
// }
#define DELAY_TIME_LIMIT_CHECKER(PARAM) \
  ::base::bindDelayTimeChecker(FROM_HERE, PARAM)

#if DCHECK_IS_ON()
#define DEBUG_DELAY_TIME_LIMIT_CHECKER(PTR_NAME) \
  DELAY_TIME_LIMIT_CHECKER(PTR_NAME)
#else
#define DEBUG_DELAY_TIME_LIMIT_CHECKER(PTR_NAME) \
  DUMMY_CHECKER(PTR_NAME)
#endif

class DelayTimeChecker
{
 public:
  static constexpr ::base::TimeDelta kMinDelayTime = ::base::TimeDelta::Min();

  static constexpr ::base::TimeDelta kMaxDelayTime = ::base::TimeDelta::Max();

  GUARD_METHOD_ON_UNKNOWN_THREAD(DelayTimeChecker)
  DelayTimeChecker(
    const ::base::Location& location
    , const ::base::TimeDelta& limitDelayTime)
    : startDelayTime_(base::Time::Now())
    , limitDelayTime_(limitDelayTime)
    , location_(location)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(DelayTimeChecker);

    DCHECK_GE(limitDelayTime_, kMinDelayTime)
      << location_.ToString()
      << " Delay time limit must be >= 0";
  }

  // check call count on callback destruction
  ~DelayTimeChecker()
  {
  }

  DelayTimeChecker(DelayTimeChecker&& other)
    : startDelayTime_{base::rvalue_cast(other.startDelayTime_)}
    , limitDelayTime_{base::rvalue_cast(other.limitDelayTime_)}
    , location_{base::rvalue_cast(other.location_)}
  {
  }

  DelayTimeChecker& operator=(
    DelayTimeChecker&& other)
  {
    startDelayTime_ = ::base::rvalue_cast(other.startDelayTime_);
    limitDelayTime_ = ::base::rvalue_cast(other.limitDelayTime_);
    location_ = ::base::rvalue_cast(other.location_);
    return *this;
  }

  void runCheckBeforeInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckBeforeInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckBeforeInvoker);

    ::base::TimeDelta elapsedTime
      = ::base::TimeDelta(base::Time::Now() - startDelayTime_);

    DCHECK_LE(elapsedTime, kMaxDelayTime)
      << location_.ToString()
      << " Unable to represent delay time in ::base::TimeDelta";

    CHECK_LE(elapsedTime, limitDelayTime_)
      << location_.ToString()
      << "\n Started execution at: "
      << startDelayTime_
      << "\n Real execution time: "
      << elapsedTime
      << "\n Delay time limit: "
      << limitDelayTime_;
  }

  void runCheckAfterInvoker()
  GUARD_METHOD_ON_UNKNOWN_THREAD(runCheckAfterInvoker)
  {
    DCHECK_METHOD_RUN_ON_UNKNOWN_THREAD(runCheckAfterInvoker);
  }

 private:
  ::base::Time startDelayTime_;

  ::base::TimeDelta limitDelayTime_;

  ::base::Location location_;

  // Object construction can be on any thread
  CREATE_METHOD_GUARD(DelayTimeChecker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckBeforeInvoker);

  // can be called on any thread
  CREATE_METHOD_GUARD(runCheckAfterInvoker);

  DISALLOW_COPY_AND_ASSIGN(DelayTimeChecker);
};

DelayTimeChecker bindDelayTimeChecker(
  const ::base::Location& location
  , const ::base::TimeDelta& val)
{
  return DelayTimeChecker{
    location
    , val
  };
}

} // namespace base
