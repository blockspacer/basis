## Key concepts: Fail points

* Fail points are used to add code points where errors may be injected in a user controlled fashion. Fail point is a code snippet that is only executed when the corresponding failpoint is active.
* Useful for fault-injection testing.

See `StrongFailPoint` in `fail_point.hpp`.

## What to use: unit tests (mocking) or fail points?

Unlike unit tests fail points can be used in production servers (for example, to test failure recovery, load balancing, etc.)

## What to use: `plug_point` or `fail_point`?

`Fail point` is like `bool`. Perfect for simple condition.

If you want to perform complex logic in custom callback, than use `plug point`.
