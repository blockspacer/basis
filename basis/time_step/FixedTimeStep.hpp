#pragma once

#include <chrono>
#include <base/logging.h> // for DCHECK

namespace basis {

  // (1 second = 1000000000 nanoseconds) 1000000000/1 = 1000000000
  constexpr size_t k1fpsNS{1000000000};

  // (1 second = 1000000000 nanoseconds) 1000000000/30 = 33333333
  constexpr size_t k30fpsNS{33333333};

  // (1 second = 1000000000 nanoseconds) 1000000000/60 = 16666666
  constexpr size_t k60fpsNS{16666666};

  constexpr std::chrono::nanoseconds k1fps{k1fpsNS};

  constexpr std::chrono::nanoseconds k30fps{k30fpsNS};

  constexpr std::chrono::nanoseconds k60fps{k60fpsNS};

  /// \note sleep() is not going to give you a fixed step loop!
  /// sleep() is nice because it saves CPU,
  /// but sleep will only have a precision of 14-15 ms on most systems,
  /// even if you only sleep for 0 or 1 ms.
  class FixedTimeStep
  {
  public:
    using delta_time_t = float;
    using clock = std::chrono::steady_clock;

  public:
    FixedTimeStep(const std::chrono::nanoseconds& tickrate = k60fps) noexcept;

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    inline /* `inline` to eleminate function call overhead */
    void update_clock(const clock::time_point& tp) noexcept {
      start_ = tp;
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    inline /* `inline` to eleminate function call overhead */
    void increase_lag(
      const std::chrono::nanoseconds& deltaTime) noexcept {
      lag_ += deltaTime;
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    [[nodiscard]] /* do not ignore return value */
    inline /* `inline` to eleminate function call overhead */
    std::chrono::nanoseconds elapsed_dt_since(
      const clock::time_point& tp) const noexcept {
      DCHECK(start_ != std::chrono::time_point<clock>::max());
      return
        std::chrono::duration_cast<std::chrono::nanoseconds>(
          tp - start_);
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    [[nodiscard]] /* do not ignore return value */
    inline /* `inline` to eleminate function call overhead */
    bool is_update_required() const noexcept {
      return (lag_ >= fps_);
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    inline /* `inline` to eleminate function call overhead */
    void update_lag() noexcept {
      lag_ -= fps_;
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    [[nodiscard]] /* do not ignore return value */
    inline /* `inline` to eleminate function call overhead */
    delta_time_t fixed_dt() const noexcept {
      return fixed_delta_time_;
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    [[nodiscard]] /* do not ignore return value */
    inline /* `inline` to eleminate function call overhead */
    std::chrono::nanoseconds lag() const noexcept {
      return lag_;
    }

    /// \note large `inline` functions cause Cache misses
    /// and affect efficiency negatively, so keep it small
    [[nodiscard]] /* do not ignore return value */
    inline /* `inline` to eleminate function call overhead */
    std::chrono::nanoseconds fixed_tickrate() const noexcept {
      return fixed_tickrate_;
    }

  private:
    /// \note lag measures how far the game clock
    /// is behind compared to the real world
    std::chrono::nanoseconds lag_{0};

    std::chrono::nanoseconds fps_;

    /// \note A higher tickrate increases the simulation precision,
    /// but also requires more CPU power and available bandwidth
    /// on both server and client.
    const delta_time_t fixed_delta_time_;

    const std::chrono::nanoseconds fixed_tickrate_;

    clock::time_point start_{std::chrono::time_point<clock>::max()};
  };

} // namespace basis
