/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once

#include <base/logging.h>

#include <cstdint>
#include <cstdlib>

namespace basis {

// Computes units per second over a given interval by tracking the units over
// each bucket of a given size and calculating the instantaneous rate assuming
// that over each bucket the rate was constant.
//
// USAGE
//
// basis::RateTracker render_fps_tracker(/*bucket_milliseconds*/ 100, /*bucket_count*/ 10u);
//
// render_fps_tracker.AddSamples(1);
//
// double fps =
//     render_fps_tracker.ComputeRateForInterval(now - last_sample_time_);
// VLOG(9) << "App.FrameTime " << rendered_fps;
//
// const int kMinRequiredSamples = 200;
// int samples = static_cast<int>(render_fps_tracker.TotalSampleCount());
// if (samples >= kMinRequiredSamples) {
//   int rendered_fps = round(render_fps_tracker.ComputeTotalRate());
//   HISTOGRAM_COUNTS_100("App.RenderFramesPerSecond",
//                            rendered_fps);
//   VLOG(9) << "App.RenderFramesPerSecond " << rendered_fps;
//   HISTOGRAM_COUNTS_100000(
//       "App.RenderSqrtPixelsPerSecond",
//       round(render_pixel_tracker_.ComputeTotalRate()));
// }
//
class RateTracker {
 public:
  RateTracker(int64_t bucket_milliseconds, size_t bucket_count);
  virtual ~RateTracker();

  // Computes the average rate over the most recent interval_milliseconds,
  // or if the first sample was added within this period, computes the rate
  // since the first sample was added.
  double ComputeRateForInterval(int64_t interval_milliseconds) const;

  // Computes the average rate over the rate tracker's recording interval
  // of bucket_milliseconds * bucket_count.
  double ComputeRate() const {
    return ComputeRateForInterval(bucket_milliseconds_ *
                                  static_cast<int64_t>(bucket_count_));
  }

  // Computes the average rate since the first sample was added to the
  // rate tracker.
  double ComputeTotalRate() const;

  // The total number of samples added.
  int64_t TotalSampleCount() const;

  // Reads the current time in order to determine the appropriate bucket for
  // these samples, and increments the count for that bucket by sample_count.
  void AddSamples(int64_t sample_count);

 protected:
  // overrideable for tests
  virtual int64_t Time() const;

 private:
  void EnsureInitialized();
  size_t NextBucketIndex(size_t bucket_index) const;

  const int64_t bucket_milliseconds_;
  const size_t bucket_count_;
  int64_t* sample_buckets_;
  size_t total_sample_count_;
  size_t current_bucket_;
  int64_t bucket_start_time_milliseconds_;
  int64_t initialization_time_milliseconds_;
};

} // namespace basis
