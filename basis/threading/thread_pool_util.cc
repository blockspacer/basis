#include "basis/threading/thread_pool_util.hpp"

#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/task/thread_pool/thread_pool.h>
#include <base/task/thread_pool/thread_pool_impl.h>
#include <base/system/sys_info.h>

#include <memory>

namespace basis {

void initThreadPool(
  const int backgroundMaxThreads
  , const int foregroundMaxThreads
  // when to reclaim idle threads
  , base::TimeDelta kSuggestedReclaimTime
){
  DCHECK(!base::ThreadPool::GetInstance());

  DCHECK(backgroundMaxThreads >= 1);
  DCHECK(foregroundMaxThreads >= 1);

  // Values were chosen so that:
  // * There are few background threads.
  // * Background threads never outnumber foreground threads.
  // * The system is utilized maximally by foreground threads.
  // * The main thread is assumed to be busy, cap foreground workers at
  //   |num_cores - 1|.
  const int num_cores
    = base::SysInfo::NumberOfProcessors();

  if(num_cores < backgroundMaxThreads)
  {
    LOG(WARNING)
      << "(low grade CPU or bad config)"
      << " num_cores < background max threads."
      << " Where"
      << " background max threads = " << backgroundMaxThreads
      << " num_cores = " << num_cores;
  }

  if(num_cores < foregroundMaxThreads)
  {
    LOG(WARNING)
      << "(low grade CPU or bad config)"
      << " num_cores < foreground max threads."
      << " Where"
      << " foreground max threads = " << foregroundMaxThreads
      << " num_cores = " << num_cores;
  }

  std::unique_ptr<base::ThreadPool> thread_pool_
    = std::make_unique<base::internal::ThreadPoolImpl>("Test");

  base::ThreadPool::SetInstance(std::move(thread_pool_));
  base::ThreadPool::GetInstance()->Start(
    base::internal::ThreadPoolImpl::InitParams{
      {backgroundMaxThreads, kSuggestedReclaimTime}
      , {foregroundMaxThreads, kSuggestedReclaimTime}
    }
    , /*worker_thread_observer_*/nullptr);
}

}  // namespace basis
