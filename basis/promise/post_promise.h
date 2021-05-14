#pragma once

#include "basic/promise/post_promise.h"

#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

namespace base {

namespace internal {

BASE_EXPORT PassedPromise
PostPromiseInternal(const boost::asio::executor& executor,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data);

BASE_EXPORT PassedPromise
PostPromiseInternal(boost::asio::io_context& context,
                    const Location& from_here,
                    internal::PromiseExecutor::Data&& executor_data);

bool PostPromiseHelperInternal(const boost::asio::executor& executor
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise);

bool PostPromiseHelperInternal(boost::asio::io_context& context
  , const Location& from_here
  , scoped_refptr<AbstractPromise> promise);

}  // namespace internal

template <typename CallbackT>
auto PostDelayedPromiseOnExecutor(const Location& from_here,
                     const boost::asio::executor& executor,
                     CallbackT task,
                     // If callback returns promise,
                     // then resolving will be done based on `nested` promise
                     // (may be not when callback finished).
                     // Must be true if callback returns promise, otherwise false.
                     // Checks if callback returns promise only in debug mode.
                     IsNestedPromise isNestedPromise = IsNestedPromise()) {
  // Extract properties from |task| callback.
  using CallbackTraits = internal::CallbackTraits<std::decay_t<CallbackT>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;

  if(isNestedPromise) {
    DCHECK(
      AllowOnlyNestedPromise<typename CallbackTraits::ReturnType>::check_passed)
        << "Nested promise not allowed. Promise posted from "
        << from_here.ToString();
  } else {
    DCHECK(
      DisallowNestedPromise<typename CallbackTraits::ReturnType>::check_passed)
        << "Nested promise not found. Promise posted from "
        << from_here.ToString();
  }

  return ReturnedPromise(
    internal::PostPromiseInternal(
     executor, from_here,
     internal::PromiseExecutor::Data(
       in_place_type_t<
         internal::PostTaskExecutor<
           typename CallbackTraits::ReturnType>>(),
       internal::ToCallbackBase(RVALUE_CAST(task))))
  );
}

// Wraps synchronous task into promise
// that will be executed when synchronous task will be done.
// That approach may not work with async tasks
// (async tasks may require ManualPromiseResolver).
// i.e. async task can return immediately and callback
// for it can be called not in proper moment in time
/**
 * \example
  DCHECK(ws_sess);
  const boost::asio::executor& executor
    = ws_sess->stream().get_executor();

  return
    somePromise()
  .ThenHere(FROM_HERE,
    ::base::BindOnce(
      /// \note returns promise,
      /// so we will wait for NESTED promise
      &PostPromiseAsio<
        ::base::OnceClosure
      >
      , FROM_HERE
      /// \note |doStartSessionAcceptor| callback
      /// must prolong lifetime of |executor|
      , COPIED(executor)
      , RVALUE_CAST(doStartSessionAcceptor)
    ) // BindOnce
  ) // ThenHere
 **/
template <typename CallbackT>
auto PostPromiseOnAsioExecutor(const Location& from_here
  , const boost::asio::executor& executor
  , CallbackT&& task
  // If callback returns promise,
  // then resolving will be done based on `nested` promise
  // (may be not when callback finished).
  // Must be true if callback returns promise, otherwise false.
  // Checks if callback returns promise only in debug mode.
  , IsNestedPromise isNestedPromise = IsNestedPromise())
{
  return PostDelayedPromiseOnExecutor(
    from_here, executor, FORWARD(task), isNestedPromise);
}

template <typename CallbackT>
auto PostDelayedPromiseOnContext(const Location& from_here,
                     boost::asio::io_context& context,
                     CallbackT task,
                     IsNestedPromise isNestedPromise = IsNestedPromise()) {
  // Extract properties from |task| callback.
  using CallbackTraits = internal::CallbackTraits<std::decay_t<CallbackT>>;
  using ReturnedPromiseResolveT = typename CallbackTraits::ResolveType;
  using ReturnedPromiseRejectT = typename CallbackTraits::RejectType;
  using ReturnedPromise =
      Promise<ReturnedPromiseResolveT, ReturnedPromiseRejectT>;

  if(isNestedPromise) {
    DCHECK(
      AllowOnlyNestedPromise<typename CallbackTraits::ReturnType>::check_passed)
        << "Nested promise not allowed. Promise posted from "
        << from_here.ToString();
  } else {
    DCHECK(
      DisallowNestedPromise<typename CallbackTraits::ReturnType>::check_passed)
        << "Nested promise not found. Promise posted from "
        << from_here.ToString();
  }

  return ReturnedPromise(
    internal::PostPromiseInternal(
     context, from_here,
     internal::PromiseExecutor::Data(
       in_place_type_t<
         internal::PostTaskExecutor<
           typename CallbackTraits::ReturnType>>(),
       internal::ToCallbackBase(RVALUE_CAST(task))))
  );
}

template <typename CallbackT>
auto PostPromiseOnAsioContext(const Location& from_here
  , boost::asio::io_context& context
  , CallbackT&& task
    // If callback returns promise,
    // then resolving will be done based on `nested` promise
    // (may be not when callback finished).
    // Must be true if callback returns promise, otherwise false.
    // Checks if callback returns promise only in debug mode.
  , IsNestedPromise isNestedPromise = IsNestedPromise())
{
  return PostDelayedPromiseOnContext(
    from_here, context, FORWARD(task), isNestedPromise);
}

}  // namespace base
