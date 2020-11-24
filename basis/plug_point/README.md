## Key concepts: Plug points

* Plug points are similar to `sigslot`
* Plug points have priority
* Useful for plugin system (plugin may want to modify original logic, not only process events).
* Plug points use `std::unique_ptr<MyPlugPoint::Subscription>` similar to `base::CallbackList`

Plug points are similar to synchronous event dispatching or sigslot i.e. single plug point can have multiple receivers.

NOTE: It is also possible to use single plug point `StrongPlugPoint` (can store only one callback) and plug point without return value (`PlugPointNotifierStorage`, can store only callback<void(...)>)

See `StrongPlugPointRunner` in `plug_point.hpp`.

## What to use: `plug points` or `entt::dispatcher`?

`plug points` can NOT delay event processing i.e. plug points are synchronous. While `entt::dispatcher` is designed so as to be used in a loop (`entt::dispatcher` allows users both to trigger immediate events or to queue events to be published all together once per tick)

Receiver in `entt::dispatcher` does NOT return any value (e.g., returns void) i.e. event system well suited to add extra logic, while plug points can modify original logic.

`plug points` can control event propagation (unlike `sigslot`). If single plug point has multiple receivers - any receiver can stop event processing (or return `base::nullopt` to continue event propagation).

## Motivation: Event propagation

Main app sends event `backup started`

Plugin `Simple backup` receives event `backup started` with priority `MEDIUM`

Plugin `Premium backup` receives event `backup started` with priority `HIGH`

`Premium backup` want to stop event event propagation, so it returns true (not `base::nullopt`).

## Usage

```cpp
// my_plug_points.hpp
using PlugPointRunner_FP1
  = ::basis::StrongPlugPointRunner<
        class PlugPointFP1Tag
        , base::Optional<bool>(int, double)
      >;

std::vector<
  std::unique_ptr<
    PlugPointRunner_FP1::Subscription
  >
> runner_subscriptions;

// on `plugin A` thread, before app started
{
  // `plugin A` includes "my_plug_points.hpp"

  PlugPointRunner_FP1* fp1
    = PlugPointRunner_FP1::GetInstance(FROM_HERE, ::basis::PlugPointName{"fp1"});

  fp1->enable();

  runner_subscriptions.push_back(fp1->addCallback(basis::PlugPointPriority::High
    , base::BindRepeating(
      [
      ](
        int a
        , double b
      ) -> base::Optional<bool>
      {
        return base::nullopt;
      }
    )));

  runner_subscriptions.push_back(fp1->addCallback(basis::PlugPointPriority::Lowest
    , base::BindRepeating(
      [
      ](
        int a
        , double b
      ) -> base::Optional<bool>
      {
        return base::nullopt;
      }
    )));
}

// on `plugin B` thread, while app is running
{
  // `plugin B` includes "my_plug_points.hpp"

  PlugPointRunner_FP1* fp1
    = PlugPointRunner_FP1::GetInstance(FROM_HERE, ::basis::PlugPointName{"fp1"});
  const base::Optional<bool> pluggedReturn = fp1->RunUntilHasValue(int{1}, double{3.0});
  if(UNLIKELY(pluggedReturn))
  {
    return pluggedReturn.value();
  }
}
```
