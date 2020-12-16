/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "tests_common.h"

#include "basis/rate/moving_average.hpp"

namespace basis {

TEST(MovingAverageTest, EmptyAverage) {
  basis::MovingAverage moving_average(1);
  EXPECT_EQ(0u, moving_average.Size());
  EXPECT_EQ(base::nullopt, moving_average.GetAverageRoundedDown());
}

// Test single value.
TEST(MovingAverageTest, OneElement) {
  basis::MovingAverage moving_average(1);
  moving_average.AddSample(3);
  EXPECT_EQ(1u, moving_average.Size());
  EXPECT_EQ(3, *moving_average.GetAverageRoundedDown());
}

TEST(MovingAverageTest, GetAverage) {
  basis::MovingAverage moving_average(1024);
  moving_average.AddSample(1);
  moving_average.AddSample(1);
  moving_average.AddSample(3);
  moving_average.AddSample(3);
  EXPECT_EQ(*moving_average.GetAverageRoundedDown(), 2);
  EXPECT_EQ(*moving_average.GetAverageRoundedToClosest(), 2);
}

TEST(MovingAverageTest, GetAverageRoundedDownRounds) {
  basis::MovingAverage moving_average(1024);
  moving_average.AddSample(1);
  moving_average.AddSample(2);
  moving_average.AddSample(2);
  moving_average.AddSample(2);
  EXPECT_EQ(*moving_average.GetAverageRoundedDown(), 1);
}

TEST(MovingAverageTest, GetAverageRoundedToClosestRounds) {
  basis::MovingAverage moving_average(1024);
  moving_average.AddSample(1);
  moving_average.AddSample(2);
  moving_average.AddSample(2);
  moving_average.AddSample(2);
  EXPECT_EQ(*moving_average.GetAverageRoundedToClosest(), 2);
}

TEST(MovingAverageTest, Reset) {
  basis::MovingAverage moving_average(5);
  moving_average.AddSample(1);
  EXPECT_EQ(1, *moving_average.GetAverageRoundedDown());
  EXPECT_EQ(1, *moving_average.GetAverageRoundedToClosest());

  moving_average.Reset();

  EXPECT_FALSE(moving_average.GetAverageRoundedDown());
  moving_average.AddSample(10);
  EXPECT_EQ(10, *moving_average.GetAverageRoundedDown());
  EXPECT_EQ(10, *moving_average.GetAverageRoundedToClosest());
}

TEST(MovingAverageTest, ManySamples) {
  basis::MovingAverage moving_average(10);
  for (int i = 1; i < 11; i++) {
    moving_average.AddSample(i);
  }
  EXPECT_EQ(*moving_average.GetAverageRoundedDown(), 5);
  EXPECT_EQ(*moving_average.GetAverageRoundedToClosest(), 6);
  for (int i = 1; i < 2001; i++) {
    moving_average.AddSample(i);
  }
  EXPECT_EQ(*moving_average.GetAverageRoundedDown(), 1995);
  EXPECT_EQ(*moving_average.GetAverageRoundedToClosest(), 1996);
}

} // namespace basis
