#pragma once

#include "basis/time_step/FixedTimeStep.hpp"
#include <chrono>
#include <functional>
#include <base/logging.h> // for DCHECK

namespace basis {

// Global EstimateMemoryUsage(T) that just calls T::EstimateMemoryUsage().
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto spareCycleBeforeUpdateCallback(Args... args)
    -> decltype(T::spareCycleBeforeUpdateCallback(std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
      std::is_same<
        decltype(T::spareCycleBeforeUpdateCallback(std::forward<Args>(args)...))
        , void>::value,
      "'T::spareCycleBeforeUpdateCallback() const' must return void.");
  return T::spareCycleBeforeUpdateCallback(std::forward<Args>(args)...);
}

// Global EstimateMemoryUsage(T) that just calls T::EstimateMemoryUsage().
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto updateCallback(Args... args)
    -> decltype(T::updateCallback(std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
      std::is_same<
        decltype(T::updateCallback(std::forward<Args>(args)...))
        , void>::value,
      "'T::updateCallback() const' must return void.");
  return T::updateCallback(std::forward<Args>(args)...);
}

// Global EstimateMemoryUsage(T) that just calls T::EstimateMemoryUsage().
template <class T, typename... Args>
inline /* `inline` to eleminate function call overhead */
auto spareCycleAfterUpdateCallback(Args... args)
    -> decltype(T::spareCycleAfterUpdateCallback(std::forward<Args>(args)...))
{
  ///\note checks return type
  static_assert(
      std::is_same<
        decltype(T::spareCycleAfterUpdateCallback(std::forward<Args>(args)...))
        , void>::value,
      "'T::spareCycleAfterUpdateCallback() const' must return void.");
  return T::spareCycleAfterUpdateCallback(std::forward<Args>(args)...);
}

/// \brief Fixed Time Step based on code from
/// https://gameprogrammingpatterns.com/game-loop.html
/// \note single `frame` may contain zero or more `update ticks`
/// \code // original code
/// double previous_timestamp = getCurrentTime();
/// double lag = 0.0;
/// while (true) // single `frame` may contain zero or more `update ticks`
/// {
///   double starting_timestamp = getCurrentTime();
///   double elapsed = starting_timestamp - previous_timestamp;
///   previous_timestamp = starting_timestamp;
///   lag += elapsed;
///
///   processInput();
///
///   while (lag >= MS_PER_UPDATE) // single `update tick`
///   {
///     update(); // must take less time than MS_PER_UPDATE
///     lag -= MS_PER_UPDATE;
///   }
///
///   render(lag / MS_PER_UPDATE);
///
///   // Mobile games and servers are more focused on the quality of gameplay
///   // than they are on maximizing the detail of the graphics, so
///   // can use sleep(time_to_next_frame) here if set an upper limit
///   // on the frame rate (usually 60 FPS)
///   // By simply sleeping for a few milliseconds instead of trying to
///   // cram ever more processing into each tick, you save battery power.
///   double ending_timestamp = getCurrentTime();
///   double total_frame_time_elapsed = ending_timestamp - starting_timestamp;
///   if (total_frame_time_elapsed < MS_PER_UPDATE) {
///     double time_to_next_frame = MS_PER_UPDATE - total_frame_time_elapsed;
///     sleep(time_to_next_frame);
///   }
/// }
template<typename UpdateCallbacksType>
class FixedTimeStepLoop {
public:
  /*using UpdateCallback
    = std::function<void(const std::chrono::nanoseconds&
                         , const std::chrono::nanoseconds&)>;
  using SpareCycleBeforeUpdateCallback
    = std::function<void(const std::chrono::nanoseconds&
                         , const std::chrono::nanoseconds&)>;
  using SpareCycleAfterUpdateCallback
    = std::function<void(const std::chrono::nanoseconds&
                         , const std::chrono::nanoseconds&
                         , const std::chrono::nanoseconds&
                         , const std::chrono::steady_clock::time_point&)>;*/

public:
  FixedTimeStepLoop(
      const std::chrono::nanoseconds& tickrate = k60fps
      , void* data_raw = nullptr)
    : time_step_{tickrate}
      , data_raw_(data_raw)
  {
    /// \note don`t divide to 0
    DCHECK(tickrate > std::chrono::nanoseconds{0});
  }

  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  [[nodiscard]] /* do not ignore return value */
  inline /* `inline` to eleminate function call overhead */
  basis::FixedTimeStep& time_step_ref() noexcept {
    return time_step_;
  }

  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void run_once() noexcept {
    const std::chrono::steady_clock::time_point frame_start_timestamp
      = std::chrono::steady_clock::now();

    /// \note amount of real time has elapsed since
    /// the last turn of the game loop.
    /// This is how much game time we need to simulate
    /// for the game�s �now� to catch up with the player�s.
    const std::chrono::nanoseconds deltaTime
      = time_step_.elapsed_dt_since(frame_start_timestamp);

    time_step_.update_clock(frame_start_timestamp);
    time_step_.increase_lag(deltaTime);

    //DCHECK(spareCycleBeforeUpdateCallback);
    //if(spareCycleBeforeUpdateCallback)
    {
#if !defined(NDEBUG)
      const auto sc_start = std::chrono::steady_clock::now();
#endif // NDEBUG

      spareCycleBeforeUpdateCallback<UpdateCallbacksType>(
        data_raw_
        , deltaTime
        , time_step_.fixed_tickrate());
      //UpdateCallbacksType::spareCycleBeforeUpdateCallback(
      //  data_raw_
      //  , deltaTime
      //  , time_step_.fixed_tickrate());
      //spareCycleBeforeUpdateCallback(deltaTime
      //                               , time_step_.fixed_tickrate());

#if !defined(NDEBUG)
      const auto sc_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            sc_start - std::chrono::steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(sc_elapsed < std::chrono::seconds{1});
#endif // NDEBUG
    }

    while (time_step_.is_update_required()) {
      //DCHECK(updateCallback);
      //if(updateCallback)
      {
#if !defined(NDEBUG)
        const auto updateStart = std::chrono::steady_clock::now();
#endif // NDEBUG

        //updateCallback(
        //  deltaTime
        //  , time_step_.fixed_tickrate());

        /// \note may be inlined
        updateCallback<UpdateCallbacksType>(
          data_raw_
          , deltaTime
          , time_step_.fixed_tickrate());

#if !defined(NDEBUG)
        const auto updateElapsed = std::chrono::duration_cast<std::chrono::seconds>(
              updateStart - std::chrono::steady_clock::now());

        /// \note Make sure that code does not take too much time
        DCHECK(updateElapsed < std::chrono::seconds{1});

        /// \note time step must be greater
        /// than the time it takes to process an update(),
        /// even on the slowest hardware.
        /// Otherwise, your game simply can�t catch up.
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

    //DCHECK(spareCycleAfterUpdateCallback);
    //if(spareCycleAfterUpdateCallback)
    {
#if !defined(NDEBUG)
      const auto sc_start = std::chrono::steady_clock::now();
#endif // NDEBUG

      spareCycleAfterUpdateCallback<UpdateCallbacksType>(
        data_raw_
        , remaining_lag
        , deltaTime
        , time_step_.fixed_tickrate()
        , frame_start_timestamp);

      //spareCycleAfterUpdateCallback(remaining_lag
      //                              , deltaTime
      //                              , time_step_.fixed_tickrate()
      //                              , frame_start_timestamp);

#if !defined(NDEBUG)
      const auto sc_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            sc_start - std::chrono::steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(sc_elapsed < std::chrono::seconds{1});
#endif // NDEBUG
    }
  }

  //[[nodiscard]] /* do not ignore return value */
  //bool run() noexcept;
  //
  //[[nodiscard]] /* do not ignore return value */
  //bool stop() noexcept;

  [[nodiscard]] /* do not ignore return value */
  bool run() noexcept {
    is_running_ = true;
    time_step_.update_clock(basis::FixedTimeStep::clock::now());
    while (is_running_) {
  #if !defined(NDEBUG)
      const auto tp_start = std::chrono::steady_clock::now();
  #endif // NDEBUG

      run_once();

  #if !defined(NDEBUG)
      const auto tp_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            tp_start - std::chrono::steady_clock::now());

      /// \note Make sure that code does not take too much time
      DCHECK(tp_elapsed < std::chrono::seconds{1});
  #endif // NDEBUG
    }
    return /* done without error */ true;
  }

  [[nodiscard]] /* do not ignore return value */
  bool stop() noexcept {
    is_running_ = false;
    return /* done without error */ true;
  }

public:
  /// \note usually you want to update world/scenes here
  /// \note usually runs at 60 FPS
  //UpdateCallback updateCallback;

  /// \note usually you want to update input system here
  //SpareCycleBeforeUpdateCallback spareCycleBeforeUpdateCallback;

  /// \note usually you want to update graphical system here
  //SpareCycleAfterUpdateCallback spareCycleAfterUpdateCallback;

  void* data_raw_;

private:
  basis::FixedTimeStep time_step_;

  bool is_running_{false};
};

} // namespace basis
