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
STRONG_FAIL_POINT(FailPoint_RecievedData);
GET_FAIL_POINT(failPointPtr, flexnet::ws::FailPoint_RecievedData);
RETURN_IF_FAIL_POINT_FAIL(failPointPtr, REFERENCED(message));
```
