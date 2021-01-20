## Key concepts: Fail points

* Fail points are used to add code points where errors may be injected in a user controlled fashion. Fail point is a code snippet that is only executed when the corresponding failpoint is active.
* Useful for fault-injection testing.
* Useful for plugin system (plugin may want to modify original logic, not only process events).

See `StrongFailPoint` in `fail_point.hpp`.

## What to use: unit tests (mocking) or fail points?

Unlike unit tests fail points can be used in production servers (for example, to test failure recovery, load balancing, etc.)

## What to use: `plug_point` or `fail_point`?

`Fail point` is like `bool`. Perfect for simple condition.

`fail points` can not return custom data or recieve input data (`fail point` is like simple bool).

So use `plug points` when you need to return custom data or recieve input data.

If you want to perform complex logic in custom callback, than use `plug point`.

## TIPS AND TRICKS: use macros

```cpp
#include "basis/basis/fail_point/fail_point.hpp"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"

namespace my_ns {
STRONG_FAIL_POINT(FailPoint_RecievedData);
} // namespace my_ns

// ...

namespace switches {

extern const char kMyFailPoint[];

}  // namespace switches

// ...

namespace switches {

// Total number of shards. Must be the same for all shards.
const char switches::kMyFailPoint[] =
    "my-fp";

}  // namespace switches

// ...

const base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
if (command_line->HasSwitch(switches::kMyFailPoint)) {
 my_ns::FailPoint_RecievedData->setFailure();
 my_ns::FailPoint_RecievedData->enable();
}

// ...

// NOTE: Avoid `ASSIGN_FAIL_POINT`, prefer to cache pointer using `FAIL_POINT_INSTANCE`
ASSIGN_FAIL_POINT(failPointPtr, my_ns::FailPoint_RecievedData);

// ...

// or `RETURN_IF_FAIL_POINT_FAIL(failPointPtr, REFERENCED(message));`
if(UNLIKELY(failPointPtr->checkFail()))
{
  // ...
}
```

## FAQ

* How to create event that's expected to happen once?

Use `base::OneShotEvent`
