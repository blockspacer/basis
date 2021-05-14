#pragma once

#include "basic/annotations/guard_annotations.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

namespace basis {

// Allows to use `boost::asio::strand` with clang thread-safety annotations like `GUARDED_BY`.
// See http://clang.llvm.org/docs/ThreadSafetyAnalysis.html
template <typename Executor>
using AnnotatedStrand
  = ::basic::AnnotateLockable<
      ::boost::asio::strand<Executor>
    >;

// Helper class used by DCHECK_RUN_ON_STRAND
class SCOPED_LOCKABLE StrandCheckerScope {
 public:
  template <typename Executor>
  explicit StrandCheckerScope(
    const ::basic::AnnotateLockable<boost::asio::strand<Executor>>* thread_like_object)
      EXCLUSIVE_LOCK_FUNCTION(thread_like_object) {}

  StrandCheckerScope(
    const StrandCheckerScope&) = delete;

  StrandCheckerScope& operator=(
    const StrandCheckerScope&) = delete;

  ~StrandCheckerScope() UNLOCK_FUNCTION() {}
};

// Type of `x` is `basis::AnnotatedStrand&`
//
// EXAMPLE
//
// using ExecutorType
//   = StreamType::executor_type;
//
// using StrandType
//   = ::boost::asio::strand<ExecutorType>;
//
// // |stream_| and calls to |async_*| are guarded by strand
// ::basis::AnnotatedStrand<ExecutorType> perConnectionStrand_
//   SET_CUSTOM_THREAD_GUARD_WITH_CHECK(
//     perConnectionStrand_
//     // 1. It safe to read value from any thread
//     // because its storage expected to be not modified.
//     // 2. On each access to strand check that stream valid
//     // otherwise `::boost::asio::post` may fail.
//     , ::base::BindRepeating(
//       [
//       ](
//         bool is_stream_valid
//         , StreamType& stream
//       ){
//         /// \note |perConnectionStrand_|
//         /// is valid as long as |stream_| valid
//         /// i.e. valid util |stream_| moved out
//         /// (it uses executor from stream).
//         return is_stream_valid;
//       }
//       , is_stream_valid_.load()
//       , REFERENCED(stream_.value())
//     ));
//
// DCHECK_RUN_ON_STRAND(&perConnectionStrand_, ExecutorType);
//
#define DCHECK_RUN_ON_STRAND(x, Type)                                              \
  ::basic::StrandCheckerScope strand_check_scope(x); \
  DCHECK((x)); \
  DCHECK((x)->data.running_in_this_thread())

} // namespace basis
