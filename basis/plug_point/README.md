## Key concepts: Plug points

* By default plug points WILL appear in the final binary (but can be disabled using macros)
* Useful for plugin system (plugin may want to modify original logic, not only process events).

Plug points are similar to synchronous event dispatching or sigslot i.e. single plug point can have multiple receivers.

## What to use: `plug points` or `entt::dispatcher`?

`plug points` can NOT delay event processing i.e. plug points are synchronous. While `entt::dispatcher` is designed so as to be used in a loop (`entt::dispatcher` allows users both to trigger immediate events or to queue events to be published all together once per tick)

Receiver in `entt::dispatcher` does NOT return any value (e.g., returns void) i.e. event system well suited to add extra logic, while plug points can modify original logic.

If single plug point has multiple receivers - any receiver can stop event processing (event propagation).

## Event propagation

Example:

Main app sends event `backup started`

Plugin `Simple backup` receives event `backup started` with priority `MEDIUM`

Plugin `Premium backup` receives event `backup started` with priority `HIGH`

Receiver can know about other registered receivers.

Plugin `Simple backup` finds that `Premium backup` already has event receiver with higher priority.

Plugin `Simple backup` decides to skip event processing.

Only plugin `Premium backup` will process event.
