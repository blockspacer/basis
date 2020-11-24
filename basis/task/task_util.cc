#include "task_util.hpp" // IWYU pragma: associated

#include <base/macros.h>
#include <base/callback.h>
#include <base/synchronization/waitable_event.h>
#include <base/bind.h>

namespace basis {

::boost::beast::detail::bind_front_wrapper<
    typename std::decay<
      std::function<void(base::OnceClosure&&)>
    >::type,
    typename std::decay<
      ::base::OnceClosure
    >::type> bindFrontOnceClosure(
  ::base::OnceClosure&& task)
{
  // Because `std::bind` arg(s) are passed by value as Lvalues
  // we need to use `bind_front_handler`.
  // There is no problem with `bind_front_handler`
  // because in this implementation all data members
  // of generated functor are forwarded to a target.
  // https://stackoverflow.com/a/61422348
  return ::boost::beast::bind_front_handler<
    std::function<void(base::OnceClosure&&)>
    , ::base::OnceClosure
  >([
    ](
      ::base::OnceClosure&& boundTask
    ) mutable {
      DCHECK(boundTask);
      ::base::rvalue_cast(boundTask).Run();
    }
    , ::base::rvalue_cast(task)
  );
}

bool RunsTasksInAnySequenceOf(
  const std::vector<scoped_refptr<::base::SequencedTaskRunner> > &task_runners
  , bool dcheck_not_empty)
{
  DCHECK(dcheck_not_empty
         ? !task_runners.empty()
         : true);
  for(size_t i = 0; i < task_runners.size(); i++) {
    DCHECK(task_runners[i]);
    if(task_runners[i]->RunsTasksInCurrentSequence()){
      return true;
    }
  }
  return false;
}

void PostTaskAndWait(const ::base::Location& from_here
  , ::base::SequencedTaskRunner* task_runner
  , ::base::OnceClosure task)
{
  ::base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL
    , ::base::WaitableEvent::InitialState::NOT_SIGNALED);
  {
    bool ok = task_runner->PostTask(from_here, std::move(task));
    DCHECK(ok);
  }
  {
    /// \note task will be executed
    /// after previous due to usage of |base::SequencedTaskRunner|
    bool ok = task_runner->PostTask(FROM_HERE,
      ::base::BindOnce(&base::WaitableEvent::Signal, ::base::Unretained(&event)));
    DCHECK(ok);
  }
  // The SequencedTaskRunner guarantees that
  // |event| will only be signaled after |task| is executed.
  event.Wait();
}

base::OnceClosure bindToTaskRunner(
  const ::base::Location& from_here,
  ::base::OnceClosure&& task,
  scoped_refptr<::base::SequencedTaskRunner> task_runner,
  ::base::TimeDelta delay) NO_EXCEPTION
{
  DCHECK(task)
    << from_here.ToString();

  DCHECK(task_runner)
    << from_here.ToString();

  return ::base::BindOnce(
    [
    ](
      const ::base::Location& from_here,
      ::base::OnceClosure&& task,
      scoped_refptr<::base::SequencedTaskRunner> task_runner,
      ::base::TimeDelta delay
    ){
      if(task_runner->RunsTasksInCurrentSequence())
      {
        std::move(task).Run();
        return;
      }

      task_runner->PostDelayedTask(from_here,
        std::move(task),
        delay
      );
    }
    , from_here
    , ::base::Passed(std::move(task))
    , task_runner
    , delay
  );
}

} // namespace basis
