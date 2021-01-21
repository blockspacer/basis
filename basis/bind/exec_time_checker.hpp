#pragma once

#include <base/rvalue_cast.h>
#include <base/bind.h>
#include <base/time/time.h>

#include <basis/checks_and_guard_annotations.hpp>

namespace base {

void perSequenceStoreTimeBeforeCallbackExecution();

base::Time perSequenceGetTimeBeforeCallbackExecution();

void perSequenceClearTimeBeforeCallbackExecution();

// Check that callback will be executed within `PARAM` `base::TimeDelta`.
// Check of execution time will be performed on each call.
//
/// \note Check will be performed on callback 'body',
/// not on time delay before callback call
/// i.e. you can use it to check performance limits.
//
// USAGE
//
// class TmpClass
// {
//  public:
//   void TestMe(const ::base::Location& location)
//   {
//     // ok: EXEC_TIME_LIMIT_CHECKER < Seconds(2)
//     ::base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(2));
//   }
// };
// {
//   ::base::RepeatingCallback<void(const ::base::Location& location)> repCb
//     = ::base::bindCheckedRepeating(
//         DEBUG_BIND_CHECKS(
//           EXEC_TIME_LIMIT_CHECKER(base::TimeDelta::FromSeconds(3))
//         )
//         , &TmpClass::TestMe
//         , ::base::Unretained(&tmpClass)
//     );
//
//   repCb.Run(FROM_HERE);
//
//   task_runner->PostTask(
//     FROM_HERE
//     , ::base::BindOnce(
//         repCb
//         , FROM_HERE
//       )
//   );
// }
//
/// \note See also `HangWatchScopeEnabled` in `base/threading/`
//
#define EXEC_TIME_LIMIT_CHECKER(PARAM) \
  ::base::bindExecTimeChecker(FROM_HERE, PARAM)

#if DCHECK_IS_ON()
#define DEBUG_EXEC_TIME_LIMIT_CHECKER(PTR_NAME) \
  EXEC_TIME_LIMIT_CHECKER(PTR_NAME)
#else
#define DEBUG_EXEC_TIME_LIMIT_CHECKER(PTR_NAME) \
  DUMMY_CHECKER(PTR_NAME)
#endif

class ExecTimeChecker
{
 public:
  static constexpr ::base::TimeDelta kMinExecTime = ::base::TimeDelta::Min();

  static constexpr ::base::TimeDelta kMaxExecTime = ::base::TimeDelta::Max();

  ExecTimeChecker(
    const ::base::Location& location
    , const ::base::TimeDelta& limitExecTime)
    : limitExecTime_(limitExecTime)
    , location_(location)
  {
    DCHECK_GE(limitExecTime_, kMinExecTime)
      << location_.ToString()
      << " Execution time limit must be >= 0";
  }

  // check call count on callback destruction
  ~ExecTimeChecker() {}

  ExecTimeChecker(ExecTimeChecker&& other)
    : limitExecTime_{RVALUE_CAST(other.limitExecTime_)}
    , location_{RVALUE_CAST(other.location_)}
  {}

  ExecTimeChecker& operator=(
    ExecTimeChecker&& other)
  {
    limitExecTime_ = RVALUE_CAST(other.limitExecTime_);
    location_ = RVALUE_CAST(other.location_);
    return *this;
  }

  void runCheckBeforeInvoker()
  {
    perSequenceStoreTimeBeforeCallbackExecution();
  }

  void runCheckAfterInvoker()
  {
    ::base::Time startExecTime
      = perSequenceGetTimeBeforeCallbackExecution();

    ::base::TimeDelta elapsedTime
      = ::base::TimeDelta(base::Time::Now() - startExecTime);

    DCHECK_LE(elapsedTime, kMaxExecTime)
      << location_.ToString()
      << " Unable to represent execution time in ::base::TimeDelta";

    DCHECK_GE(elapsedTime, kMinExecTime)
      << location_.ToString()
      << " Execution time must be >= 0";

    CHECK_LE(elapsedTime, limitExecTime_)
      << location_.ToString()
      << "\n Started execution at: "
      << startExecTime
      << "\n Real execution time: "
      << elapsedTime
      << "\n Execution time limit: "
      << limitExecTime_;

    perSequenceClearTimeBeforeCallbackExecution();
  }

 private:
  ::base::TimeDelta limitExecTime_;

  ::base::Location location_;

  DISALLOW_COPY_AND_ASSIGN(ExecTimeChecker);
};

ExecTimeChecker bindExecTimeChecker(
  const ::base::Location& location
  , const ::base::TimeDelta& val);

} // namespace base
