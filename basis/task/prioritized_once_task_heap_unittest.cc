#include "tests_common.h"

#include "basis/task/prioritized_once_task_heap.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include "base/rvalue_cast.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_restrictions.h"

namespace basis {
namespace {

class PrioritizedOnceTaskHeapTest : public testing::Test {
 public:
  PrioritizedOnceTaskHeapTest() {}

  void PushTaskName(const std::string& task_name) {
    base::AutoLock auto_lock(task_names_lock_);
    task_names_.push_back(task_name);
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::vector<std::string> task_names_;
  base::Lock task_names_lock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrioritizedOnceTaskHeapTest);
};

void runWithThreadCheck(scoped_refptr<base::TaskRunner> expected_task_runner,
                        base::OnceClosure callback)
{
  EXPECT_TRUE(expected_task_runner->RunsTasksInCurrentSequence());
  RVALUE_CAST(callback).Run();
}

TEST_F(PrioritizedOnceTaskHeapTest, OnceCbOnThread) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap =
      base::MakeRefCounted<PrioritizedOnceTaskHeap>(
        true // with_thread_locking
      );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindOnce(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task5")
    , 5 // priority
  );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindOnce(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task7")
    , 7 // priority
  );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindOnce(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task3")
    , 3 // priority
  );

  base::RunLoop run_loop;

  task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce([
        ](
          scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap
        ){
          prioritized_task_heap->RunAllTasks();
          EXPECT_EQ(prioritized_task_heap->size(), 0u);
        }
        , prioritized_task_heap
      )
      , base::BindOnce(runWithThreadCheck
          , scoped_task_environment_.GetMainThreadTaskRunner()
          , run_loop.QuitClosure())
  );

  run_loop.Run();

  EXPECT_EQ((std::vector<std::string>{"Task3", "Task5", "Task7"}), task_names_);
}

TEST_F(PrioritizedOnceTaskHeapTest, RepeatingCbOnThread) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap =
      base::MakeRefCounted<PrioritizedOnceTaskHeap>(
        true // with_thread_locking
      );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task5")
    , 5 // priority
  );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task7")
    , 7 // priority
  );

  prioritized_task_heap->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task3")
    , 3 // priority
  );

  base::RunLoop run_loop;

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce([
        ](
          scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap
          , PrioritizedOnceTaskHeapTest* self
        ){
          prioritized_task_heap->RunAllTasks();
          EXPECT_EQ(prioritized_task_heap->size(), 0u);

          // add extra task
          prioritized_task_heap->ScheduleTask(FROM_HERE
            , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                             base::Unretained(self), "Task2")
            , 2 // priority
          );
        }
        , prioritized_task_heap
        , base::Unretained(this)
      )
  );

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce([
        ](
          scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap
        ){
          prioritized_task_heap->RunAllTasks();
          EXPECT_EQ(prioritized_task_heap->size(), 0u);
        }
        , prioritized_task_heap
      )
  );

  task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce([
        ](
          scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heap
        ){
          prioritized_task_heap->RunAllTasks();
          EXPECT_EQ(prioritized_task_heap->size(), 0u);
        }
        , prioritized_task_heap
      )
      , base::BindOnce(runWithThreadCheck
          , scoped_task_environment_.GetMainThreadTaskRunner()
          , run_loop.QuitClosure())
  );

  run_loop.Run();

  EXPECT_EQ((std::vector<std::string>{"Task3", "Task5", "Task7", "Task2"}), task_names_);
}


TEST_F(PrioritizedOnceTaskHeapTest, ExtractSubHeapOnThreads) {
  auto task_runnerA =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto task_runnerB =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heapA =
      base::MakeRefCounted<PrioritizedOnceTaskHeap>(
        true // with_thread_locking
      );

  prioritized_task_heapA->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task5")
    , 5 // priority
  );

  prioritized_task_heapA->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task7")
    , 7 // priority
  );

  prioritized_task_heapA->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task3")
    , 3 // priority
  );

  prioritized_task_heapA->ScheduleTask(FROM_HERE
    , base::BindRepeating(&PrioritizedOnceTaskHeapTest::PushTaskName,
                     base::Unretained(this), "Task2")
    , 2 // priority
  );

  std::vector<PrioritizedOnceTaskHeap::Job> subHeapJobs
    = prioritized_task_heapA->extractSubHeap(1);

  scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heapB =
      base::MakeRefCounted<PrioritizedOnceTaskHeap>(
        true // with_thread_locking
        , RVALUE_CAST(subHeapJobs)
      );

  {
    base::RunLoop run_loop;

    task_runnerA->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce([
          ](
            scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heapA
          ){
            prioritized_task_heapA->RunAllTasks();
            EXPECT_EQ(prioritized_task_heapA->size(), 0u);
          }
          , prioritized_task_heapA
        )
        , base::BindOnce(runWithThreadCheck
            , scoped_task_environment_.GetMainThreadTaskRunner()
            , run_loop.QuitClosure())
    );

    run_loop.Run();

    EXPECT_EQ((std::vector<std::string>{"Task2", "Task5"}), task_names_);
  }

  task_names_.clear();

  {
    base::RunLoop run_loop;

    task_runnerB->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce([
          ](
            scoped_refptr<PrioritizedOnceTaskHeap> prioritized_task_heapB
          ){
            prioritized_task_heapB->RunAllTasks();
            EXPECT_EQ(prioritized_task_heapB->size(), 0u);
          }
          , prioritized_task_heapB
        )
        , base::BindOnce(runWithThreadCheck
            , scoped_task_environment_.GetMainThreadTaskRunner()
            , run_loop.QuitClosure())
    );

    run_loop.Run();

    EXPECT_EQ((std::vector<std::string>{"Task3", "Task7"}), task_names_);
  }
}

}  // namespace
}  // namespace basis
