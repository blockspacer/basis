## Key concepts: Fail points

* By default fail points will not appear in the final binary (using macros) i.e. not cause regular code performance regression
* Fail points are used to add code points where errors may be injected in a user controlled fashion. Fail point is a code snippet that is only executed when the corresponding failpoint is active.
* Useful for fault-injection testing.

## What to use: unit tests (mocking) or fail points?

Unlike unit tests fail points can be used in production servers (for example, to test load balancing)
