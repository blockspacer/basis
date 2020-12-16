#pragma once

#include "basis/time_step/fixed_time_step.hpp"
#include <chrono>
#include <functional>
#include <base/logging.h>

namespace basis {

// calls method `T::spareCycleBeforeUpdateCallback()`
// based on provided type `T`
// and checks that `T::spareCycleBeforeUpdateCallback()`
// has valid return type
/// \note Handle input here (server commands, keypress, e.t.c.)
/// \note This is `earlyUpdate` part of main loop:
/// while(true)
/// {
///   earlyUpdate();
///
///   while (lag >= MS_PER_UPDATE) // single `update tick`
///   {
///     simulationUpdate();
///   }
///
///   lateUpdate();
/// } // while(true)
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto spareCycleBeforeUpdateCallback(Args... args)
  -> decltype(T::spareCycleBeforeUpdateCallback(
       std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
    std::is_same<
      decltype(T::spareCycleBeforeUpdateCallback(
        std::forward<Args>(args)...))
      , void>::value,
    "'T::spareCycleBeforeUpdateCallback() const' "
    "must return void.");

  // calls method `T::spareCycleBeforeUpdateCallback()`
  return T::spareCycleBeforeUpdateCallback(
    std::forward<Args>(args)...);
}

// calls method `T::updateCallback()`
// based on provided type `T`
// and checks that `T::updateCallback()`
// has valid return type
/// \note Handle simulation here (update physics e.t.c.)
/// i.e. update world/scenes here
/// \note This is `simulationUpdate` part of main loop:
/// while(true)
/// {
///   earlyUpdate();
///
///   while (lag >= MS_PER_UPDATE) // single `update tick`
///   {
///     simulationUpdate();
///   }
///
///   lateUpdate();
/// } // while(true)
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto updateCallback(Args... args)
    -> decltype(T::updateCallback(
      std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
    std::is_same<
      decltype(T::updateCallback(
        std::forward<Args>(args)...))
      , void>::value,
    "'T::updateCallback() const' "
    "must return void.");

  // calls method `T::updateCallback()`
  return T::updateCallback(
    std::forward<Args>(args)...);
}

// calls method `T::spareCycleAfterUpdateCallback()`
// based on provided type `T`
// and checks that `T::spareCycleAfterUpdateCallback()`
// has valid return type
/// \note Handle outgoing network here (send snapshots e.t.c.)
/// or update graphical system here
/// \note This is `lateUpdate` part of main loop:
/// while(true)
/// {
///   earlyUpdate();
///
///   while (lag >= MS_PER_UPDATE) // single `update tick`
///   {
///     simulationUpdate();
///   }
///
///   lateUpdate();
/// } // while(true)
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto spareCycleAfterUpdateCallback(Args... args)
    -> decltype(T::spareCycleAfterUpdateCallback(
      std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
    std::is_same<
      decltype(T::spareCycleAfterUpdateCallback(
        std::forward<Args>(args)...))
      , void>::value,
    "'T::spareCycleAfterUpdateCallback() const' "
    "must return void.");

  // calls method `T::spareCycleAfterUpdateCallback()`
  return T::spareCycleAfterUpdateCallback(
    std::forward<Args>(args)...);
}

/// \brief Fixed Time Step based on code from
/// https://gameprogrammingpatterns.com/game-loop.html
/// \note single `frame` may contain zero or more `update ticks`
/// \code // original code
/// double previous_timestamp = getCurrentTime();
/// double lag = 0.0;
/// // single `frame` may contain zero or more `update ticks`
/// while (true)
/// {
///   double starting_timestamp = getCurrentTime();
///   double elapsed = starting_timestamp - previous_timestamp;
///   previous_timestamp = starting_timestamp;
///   lag += elapsed;
///
///   processInput(); // "early update"
///
///   while (lag >= MS_PER_UPDATE) // single `update tick`
///   {
///     // "simulation update"
///     update(); // must take less time than MS_PER_UPDATE
///     lag -= MS_PER_UPDATE;
///   }
///
///   render(lag / MS_PER_UPDATE); // "late update"
///
///   // Mobile games and servers are more focused
///   // on the quality of gameplay
///   // than they are on maximizing the detail of the graphics,
///   // so can use sleep(time_to_next_frame) here
///   // if set an upper limit
///   // on the frame rate (usually 60 FPS)
///   // By simply sleeping for a few milliseconds
///   // instead of trying to
///   // cram ever more processing into each tick,
///   // you save battery power.
///   double ending_timestamp
///     = getCurrentTime();
///   double total_frame_time_elapsed
///     = ending_timestamp - starting_timestamp;
///   if (total_frame_time_elapsed < MS_PER_UPDATE) {
///     double time_to_next_frame
///       = MS_PER_UPDATE - total_frame_time_elapsed;
///     sleep(time_to_next_frame);
///   }
/// }
template<
  /// \note we use static polymorphism for performance reasons
  // |UpdateCallbacksType| holds periodic tasks
  // that will be executed as part of main loop
  // i.e. tasks are three parts of loop:
  /// while(true)
  /// {
  ///   earlyUpdate(); // <-- 1 part
  ///
  ///   while (lag >= MS_PER_UPDATE) // single `update tick`
  ///   {
  ///     simulationUpdate(); // <-- 2 part
  ///   }
  ///
  ///   lateUpdate(); // <-- 3 part
  typename UpdateCallbacksType
>
class FixedTimeStepLoop {
public:
#if !defined(NDEBUG)
  /// \note Make sure that code does not take too much time
  static constexpr int kMaxLagSeconds = 1;
#endif // NDEBUG

  FixedTimeStepLoop(
    // |tickrate| controls `MS_PER_UPDATE` in main loop
    // i.e. in `while (lag >= MS_PER_UPDATE)`
    const std::chrono::nanoseconds& tickrate = k60fps
    // data that must be passed to provided update callbacks
    , void* data_raw = nullptr)
    : time_step_{tickrate}
    , data_raw_(data_raw)
  {
    /// \note don`t divide to 0
    DCHECK(tickrate > std::chrono::nanoseconds{0});
  }

  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  MUST_USE_RETURN_VALUE
  inline /* `inline` to eleminate function call overhead */
  ::basis::FixedTimeStep& time_step_ref() noexcept
  {
    return time_step_;
  }

  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void run_once() noexcept
  {
    using time_point
      = std::chrono::steady_clock::time_point;

    using steady_clock
      = std::chrono::steady_clock;

    const time_point frame_start_timestamp
      = steady_clock::now();

    /// \note amount of real time has elapsed since
    /// the last turn of the game loop.
    /// This is how much game time we need to simulate
    /// for the game's 'now' to catch up with the player's.
    const std::chrono::nanoseconds deltaTime
      = time_step_.elapsed_dt_since(frame_start_timestamp);

    // equal to code below from
    // https://gameprogrammingpatterns.com/game-loop.html
    ///   double starting_timestamp = getCurrentTime();
    ///   double elapsed = starting_timestamp - previous_timestamp;
    ///   previous_timestamp = starting_timestamp;
    ///   lag += elapsed;
    {
      time_step_.update_clock(frame_start_timestamp);
      time_step_.increase_lag(deltaTime);
    }

    // execute |spareCycleBeforeUpdateCallback|
    // and (only in debug mode) check execution time
    {
#if !defined(NDEBUG)
      const auto sc_start = steady_clock::now();
#endif // NDEBUG

      spareCycleBeforeUpdateCallback<UpdateCallbacksType>(
        data_raw_
        , deltaTime
        , time_step_.fixed_tickrate());

#if !defined(NDEBUG)
      const auto sc_elapsed
        = std::chrono::duration_cast<std::chrono::seconds>(
            sc_start - steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(sc_elapsed
        < std::chrono::seconds{kMaxLagSeconds});
#endif // NDEBUG
    }

    while (time_step_.is_update_required())
    {
      // execute |updateCallback|
      // and (only in debug mode) check execution time
      {
#if !defined(NDEBUG)
        const auto updateStart = steady_clock::now();
#endif // NDEBUG

        /// \note may be inlined
        updateCallback<UpdateCallbacksType>(
          data_raw_
          , deltaTime
          , time_step_.fixed_tickrate());

#if !defined(NDEBUG)
        const auto updateElapsed
          = std::chrono::duration_cast<std::chrono::seconds>(
              updateStart - steady_clock::now());

        /// \note Make sure that code does not take too much time
        DCHECK(updateElapsed
          < std::chrono::seconds{kMaxLagSeconds});

        /// \note time step must be greater
        /// than the time it takes to process an update(),
        /// even on the slowest hardware.
        /// Otherwise, your game simply can not catch up.
        DCHECK(time_step_.fixed_tickrate() > updateElapsed);
#endif // NDEBUG
      }

      time_step_.update_lag();
    }

    /// \note can be used to compute (lag / MS_PER_UPDATE), so
    /// we can use extrapolation between update() calls
    /// \note extrapolation may be wrong when it is used between update() calls,
    /// but it is less noticeable than the stuttering you get
    /// if you do not extrapolate at all.
    /// \see https://gameprogrammingpatterns.com/game-loop.html
    const std::chrono::nanoseconds remaining_lag
      = time_step_.lag();

    // execute |spareCycleAfterUpdateCallback|
    // and (only in debug mode) check execution time
    {
#if !defined(NDEBUG)
      const auto sc_start = steady_clock::now();
#endif // NDEBUG

      spareCycleAfterUpdateCallback<UpdateCallbacksType>(
        data_raw_
        , remaining_lag
        , deltaTime
        , time_step_.fixed_tickrate()
        , frame_start_timestamp);

#if !defined(NDEBUG)
      const auto sc_elapsed
        = std::chrono::duration_cast<std::chrono::seconds>(
            sc_start - steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(sc_elapsed
        < std::chrono::seconds{kMaxLagSeconds});
#endif // NDEBUG
    }
  }

  MUST_USE_RETURN_VALUE
  bool run() noexcept
  {
    using steady_clock
      = std::chrono::steady_clock;

    is_running_ = true;

    /// \note reset to valid time before
    /// first iteration of main loop
    time_step_.update_clock(basis::FixedTimeStep::clock::now());

    while (is_running_) {
  #if !defined(NDEBUG)
      const auto tp_start = steady_clock::now();
  #endif // NDEBUG

      run_once();

  #if !defined(NDEBUG)
      const auto tp_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            tp_start - steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(tp_elapsed
        < std::chrono::seconds{kMaxLagSeconds});
  #endif // NDEBUG
    }
    return /* done without error */ true;
  }

  MUST_USE_RETURN_VALUE
  bool stop() noexcept
  {
    is_running_ = false;
    return /* done without error */ true;
  }

public:
  // data that must be passed to provided update callbacks
  void* data_raw_;

private:
  ::basis::FixedTimeStep time_step_;

  bool is_running_{false};
};

} // namespace basis
