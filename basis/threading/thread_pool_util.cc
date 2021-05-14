#include "basis/threading/thread_pool_util.h" // IWYU pragma: associated

#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/task/thread_pool.h>
#include <base/system/sys_info.h>
#include "base/threading/thread_task_runner_handle.h"
#include "base/task/thread_pool/thread_pool_instance.h"

#include <basic/rvalue_cast.h>

#include <memory>

namespace basis {

void initThreadPool(
  const int max_num_foreground_threads_in
){
  DCHECK(max_num_foreground_threads_in >= 1);

  // Values were chosen so that:
  // * There are few background threads.
  // * Background threads never outnumber foreground threads.
  // * The system is utilized maximally by foreground threads.
  // * The main thread is assumed to be busy, cap foreground workers at
  //   |num_cores - 1|.
  const int num_cores
    = ::base::SysInfo::NumberOfProcessors();

  if(num_cores < max_num_foreground_threads_in)
  {
    LOG(WARNING)
      << "(low grade CPU or bad config)"
      << " num_cores < foreground max threads."
      << " Where"
      << " foreground max threads = " << max_num_foreground_threads_in
      << " num_cores = " << num_cores;
  }

  base::ThreadPoolInstance::InitParams thread_pool_init_params{
    max_num_foreground_threads_in
  };

  base::ThreadPoolInstance::Create("AppThreadPool");
  base::ThreadPoolInstance::Get()->Start(thread_pool_init_params);
}

}  // namespace basis
