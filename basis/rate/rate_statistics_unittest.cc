/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "basis/rate/rate_statistics.hpp"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace basis {

namespace basis_impl {

namespace {

RateStatistics<double> CreateStatsFilledWithIntsFrom1ToN(int n) {
  std::vector<double> data;
  for (int i = 1; i <= n; i++) {
    data.push_back(i);
  }
  absl::c_shuffle(data, std::mt19937(std::random_device()()));

  RateStatistics<double> stats;
  for (double v : data) {
    stats.AddSample(v);
  }
  return stats;
}

// Add n samples drawn from uniform distribution in [a;b].
RateStatistics<double> CreateStatsFromUniformDistribution(int n,
                                                             double a,
                                                             double b) {
  std::mt19937 gen{std::random_device()()};
  std::uniform_real_distribution<> dis(a, b);

  RateStatistics<double> stats;
  for (int i = 1; i <= n; i++) {
    stats.AddSample(dis(gen));
  }
  return stats;
}

class RateStatisticsTest : public ::testing::TestWithParam<int> {};

constexpr int SIZE_FOR_MERGE = 5;

TEST(RateStatistics, FullSimpleTest) {
  auto stats = CreateStatsFilledWithIntsFrom1ToN(100);

  EXPECT_DOUBLE_EQ(*stats.GetMin(), 1.0);
  EXPECT_DOUBLE_EQ(*stats.GetMax(), 100.0);
  // EXPECT_DOUBLE_EQ is too strict (max 4 ULP) for this one.
  ASSERT_NEAR(*stats.GetMean(), 50.5, 1e-10);
}

TEST(RateStatistics, VarianceAndDeviation) {
  RateStatistics<int> stats;
  stats.AddSample(2);
  stats.AddSample(2);
  stats.AddSample(-1);
  stats.AddSample(5);

  EXPECT_DOUBLE_EQ(*stats.GetMean(), 2.0);
  EXPECT_DOUBLE_EQ(*stats.GetVariance(), 4.5);
  EXPECT_DOUBLE_EQ(*stats.GetStandardDeviation(), sqrt(4.5));
}

TEST(RateStatistics, RemoveSample) {
  // We check that adding then removing sample is no-op,
  // or so (due to loss of precision).
  RateStatistics<int> stats;
  stats.AddSample(2);
  stats.AddSample(2);
  stats.AddSample(-1);
  stats.AddSample(5);

  constexpr int iterations = 1e5;
  for (int i = 0; i < iterations; ++i) {
    stats.AddSample(i);
    stats.RemoveSample(i);

    EXPECT_NEAR(*stats.GetMean(), 2.0, 1e-8);
    EXPECT_NEAR(*stats.GetVariance(), 4.5, 1e-3);
    EXPECT_NEAR(*stats.GetStandardDeviation(), sqrt(4.5), 1e-4);
  }
}

TEST(RateStatistics, RemoveSamplesSequence) {
  // We check that adding then removing a sequence of samples is no-op,
  // or so (due to loss of precision).
  RateStatistics<int> stats;
  stats.AddSample(2);
  stats.AddSample(2);
  stats.AddSample(-1);
  stats.AddSample(5);

  constexpr int iterations = 1e4;
  for (int i = 0; i < iterations; ++i) {
    stats.AddSample(i);
  }
  for (int i = 0; i < iterations; ++i) {
    stats.RemoveSample(i);
  }

  EXPECT_NEAR(*stats.GetMean(), 2.0, 1e-7);
  EXPECT_NEAR(*stats.GetVariance(), 4.5, 1e-3);
  EXPECT_NEAR(*stats.GetStandardDeviation(), sqrt(4.5), 1e-4);
}

TEST(RateStatistics, VarianceFromUniformDistribution) {
  // Check variance converge to 1/12 for [0;1) uniform distribution.
  // Acts as a sanity check for NumericStabilityForVariance test.
  auto stats = CreateStatsFromUniformDistribution(1e6, 0, 1);

  EXPECT_NEAR(*stats.GetVariance(), 1. / 12, 1e-3);
}

TEST(RateStatistics, NumericStabilityForVariance) {
  // Same test as VarianceFromUniformDistribution,
  // except the range is shifted to [1e9;1e9+1).
  // Variance should also converge to 1/12.
  // NB: Although we lose precision for the samples themselves, the fractional
  //     part still enjoys 22 bits of mantissa and errors should even out,
  //     so that couldn't explain a mismatch.
  auto stats = CreateStatsFromUniformDistribution(1e6, 1e9, 1e9 + 1);

  EXPECT_NEAR(*stats.GetVariance(), 1. / 12, 1e-3);
}

TEST(RateStatistics, MinRemainsUnchangedAfterRemove) {
  // We don't want to recompute min (that's RollingAccumulator's role),
  // check we get the overall min.
  RateStatistics<int> stats;
  stats.AddSample(1);
  stats.AddSample(2);
  stats.RemoveSample(1);
  EXPECT_EQ(stats.GetMin(), 1);
}

TEST(RateStatistics, MaxRemainsUnchangedAfterRemove) {
  // We don't want to recompute max (that's RollingAccumulator's role),
  // check we get the overall max.
  RateStatistics<int> stats;
  stats.AddSample(1);
  stats.AddSample(2);
  stats.RemoveSample(2);
  EXPECT_EQ(stats.GetMax(), 2);
}

TEST_P(RateStatisticsTest, MergeStatistics) {
  int data[SIZE_FOR_MERGE] = {2, 2, -1, 5, 10};
  // Split the data in different partitions.
  // We have 6 distinct tests:
  //   * Empty merged with full sequence.
  //   * 1 sample merged with 4 last.
  //   * 2 samples merged with 3 last.
  //   [...]
  //   * Full merged with empty sequence.
  // All must lead to the same result.
  // I miss QuickCheck so much.
  RateStatistics<int> stats0, stats1;
  for (int i = 0; i < GetParam(); ++i) {
    stats0.AddSample(data[i]);
  }
  for (int i = GetParam(); i < SIZE_FOR_MERGE; ++i) {
    stats1.AddSample(data[i]);
  }
  stats0.MergeStatistics(stats1);

  EXPECT_EQ(stats0.Size(), SIZE_FOR_MERGE);
  EXPECT_DOUBLE_EQ(*stats0.GetMin(), -1);
  EXPECT_DOUBLE_EQ(*stats0.GetMax(), 10);
  EXPECT_DOUBLE_EQ(*stats0.GetMean(), 3.6);
  EXPECT_DOUBLE_EQ(*stats0.GetVariance(), 13.84);
  EXPECT_DOUBLE_EQ(*stats0.GetStandardDeviation(), sqrt(13.84));
}

INSTANTIATE_TEST_SUITE_P(RateStatisticsTests,
                         RateStatisticsTest,
                         ::testing::Range(0, SIZE_FOR_MERGE + 1));

}  // namespace
}  // namespace basis_impl

}  // namespace basis
