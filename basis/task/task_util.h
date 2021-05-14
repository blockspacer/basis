#pragma once

#include <base/logging.h>
#include <base/location.h>
#include <base/callback_forward.h>
#include <base/callback_helpers.h>
#include <base/sequenced_task_runner.h>
#include <base/macros.h>

#include <basic/macros.h>
#include <basic/rvalue_cast.h>
#include <basic/promise/helpers.h>

#include <boost/beast/core.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <functional>

namespace basis {

// converts `base::OnceClosure` into
// `std::function<void()>`
::boost::beast::detail::bind_front_wrapper<
    typename std::decay<
      std::function<void(base::OnceClosure&&)>
    >::type,
    typename std::decay<
      ::base::OnceClosure
    >::type> bindFrontOnceClosure(
  ::base::OnceClosure&& task);

// converts `base::OnceCallback<T>` into
// `std::function<...(...)>`
/**
 * USAGE
  void WsChannel::onWrite(
    int a /// \note `int` bound by `base::BindOnce`
    , ErrorCode ec
    , std::size_t bytes_transferred) NO_EXCEPTION
  {
    LOG_CALL(DVLOG(99));

    // ...
  }

  ws_.async_write(
    ::boost::asio::buffer(
      *(dp.data))
    , ::boost::asio::bind_executor(
        *perConnectionStrand_
        , ::basis::bindFrontOnceCallback(
            ::base::BindOnce(
              &WsChannel::onWrite
              , ::base::Unretained(this)
              , 1 /// \note `int` bound by `base::BindOnce`
          ))
      )
  );
 */
template<
  typename RetType
  , typename... ArgsType
  , class... Args
>
auto bindFrontOnceCallback(
  ::base::OnceCallback<RetType(ArgsType...)>&& task
  , Args&&... args)
{
  using CallbackT
    = ::base::OnceCallback<RetType(ArgsType...)>;

  // Because `std::bind` arg(s) are passed by value as Lvalues
  // we need to use `bind_front_handler`.
  // There is no problem with `bind_front_handler`
  // because in this implementation all data members
  // of generated functor are forwarded to a target.
  // https://stackoverflow.com/a/61422348
  return ::boost::beast::bind_front_handler<
    std::function<RetType(CallbackT&&, ArgsType...)>
    , CallbackT
    , Args...
  >([
    ](
      CallbackT&& boundTask
      , auto... passedArgs
    ) mutable {
      DCHECK(boundTask);
      RVALUE_CAST(boundTask).Run(
        FORWARD(passedArgs)...
      );
    }
    , RVALUE_CAST(task)
    , FORWARD(args)...
  );
}

bool RunsTasksInAnySequenceOf(
  const std::vector<scoped_refptr<::base::SequencedTaskRunner>>& task_runners
  , bool dcheck_not_empty = true);

// Posts |task| to |task_runner| and blocks until it is executed.
void PostTaskAndWait(const ::base::Location& from_here
  , ::base::SequencedTaskRunner* task_runner
  , ::base::OnceClosure task);

// Redirects task to task runner.
//
// USAGE
//
//   ::base::OnceClosure task
//     = ::base::internal::bindToTaskRunner(
//         FROM_HERE,
//         ::base::BindOnce(
//             &ExampleServer::doQuit
//             , ::base::Unretained(this)),
//         base::ThreadTaskRunnerHandle::Get())
BASE_EXPORT
MUST_USE_RETURN_VALUE
base::OnceClosure bindToTaskRunner(
  const ::base::Location& from_here,
  ::base::OnceClosure&& task,
  scoped_refptr<::base::SequencedTaskRunner> task_runner,
  ::base::TimeDelta delay = ::base::TimeDelta()) NO_EXCEPTION;

} // namespace basis
