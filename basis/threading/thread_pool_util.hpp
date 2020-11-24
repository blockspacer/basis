#pragma once

#include <base/time/time.h>

#include <string>

namespace basis {

void initThreadPool(
  const int backgroundMaxThreads
  , const int foregroundMaxThreads
  // when to reclaim idle threads
  , ::base::TimeDelta kSuggestedReclaimTime
      = ::base::TimeDelta::FromSeconds(30)
);

}  // namespace basis
