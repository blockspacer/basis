/**
 * Based on code from entt v3.4.0
 * See https://github.com/skypjack/entt/blob/v3.4.0/src/entt/entity/registry.hpp
 * Made modifications to fully access `std::vector<variable_data> vars{};`
 * and perform thread-safety validations.
**/

#pragma once

#include "basis/ECS/ecs.h"
#include "basis/ECS/unsafe_context.h"

#include <entt/core/type_traits.hpp>

#include <base/macros.h>
#include <base/logging.h>
#include <base/location.h>
#include <base/sequenced_task_runner.h>
#include "base/sequence_checker.h"
#include <base/threading/thread_checker.h>
#include <base/memory/weak_ptr.h>
#include <base/memory/ref_counted.h>
#include <base/memory/scoped_refptr.h>
#include <base/macros.h>
#include "base/sequence_checker.h"

#include <basic/macros.h>
#include <basic/rvalue_cast.h>

#include <vector>
#include <string>
#include <atomic>

namespace base {

class SingleThreadTaskRunner;

template <typename T>
struct DefaultSingletonTraits;

} // namespace base

namespace ECS {

#if 0
class TLSSequenceContextStore {
 public:
  // Returns the TLSSequenceContextStore instance for this thread. Will be NULL
  // if no instance was created in this thread before.
  static TLSSequenceContextStore* current();

  TLSSequenceContextStore();
  ~TLSSequenceContextStore();

  void Set(const scoped_refptr<SequenceLocalContext>& value);

  scoped_refptr<SequenceLocalContext> value() const {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    return value_;
  }

 private:
  scoped_refptr<SequenceLocalContext> value_;

  // Thread-affine per use of TLS in impl.
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(TLSSequenceContextStore);
};

  TLSSequenceContextStore* GetTLSSequenceContextStore();
#endif

// Stores context variables that bound to single sequence.
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class SequenceLocalContext
  : public ::base::RefCountedThreadSafe<SequenceLocalContext>
{
 public:
  static SequenceLocalContext* getLocalInstance(
    const ::base::Location& from_here
    // will be used in `DCHECK(task_runner.RunsTasksInCurrentSequence());`
    , scoped_refptr<::base::SequencedTaskRunner> task_runner);

  UnsafeTypeContext& context()
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    return context_;
  }

  const UnsafeTypeContext& context() const
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    return context_;
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Component& ctx(
    const ::base::Location& from_here)
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DCHECK(context_.try_ctx_var<Component>())
      << "failed SequenceLocalContext::ctx from "
      << from_here.ToString();

    return context_.ctx_var<Component>();
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool try_ctx(
    const ::base::Location& from_here)
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

#if DCHECK_IS_ON()
    if(!context_.try_ctx_var<Component>())
    {
      // just some extra logging for debug purposes
      DVLOG(9)
        << "result SequenceLocalContext::try_ctx_var is false from "
        << from_here.ToString();
    }
#endif // DCHECK_IS_ON()

    return context_.try_ctx_var<Component>() != nullptr;
  }

  template<typename Type, typename... Args>
  MUST_USE_RETURN_VALUE
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Type& set_once(const ::base::Location& from_here
    , const std::string& debug_name
    , Args&&... args)
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DVLOG(9)
      << "called SequenceLocalContext::set from "
      << from_here.ToString()
      << " added to global context: "
      << debug_name;

    /// \note can be set only once
    DCHECK(!context_.try_ctx_var<Type>());
    return context_.set_var<Type>(
      debug_name
      , FORWARD(args)...);
  }

  template<typename Type>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void unset(const ::base::Location& from_here)
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DVLOG(9)
      << "called SequenceLocalContext::unset from "
      << from_here.ToString();

    DCHECK(context_.try_ctx_var<Type>());
    return context_.unset_var<Type>(from_here);
  }

  template<typename Type>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void try_unset(const ::base::Location& from_here)
  {
    // Unable to use global context from wrong thread
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    DVLOG(9)
      << "called SequenceLocalContext::unset from "
      << from_here.ToString();

    return context_.unset_var<Type>(from_here);
  }

 private:
  SequenceLocalContext();

  /// \note from |base::RefCounted| docs:
  /// You should always make your destructor non-public,
  /// to avoid any code deleting
  /// the object accidently while there are references to it.
  ~SequenceLocalContext();

 private:
  UnsafeTypeContext context_{};

  ::base::WeakPtrFactory<SequenceLocalContext> weak_ptr_factory_{this};

  friend class ::base::RefCountedThreadSafe<SequenceLocalContext>;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SequenceLocalContext);
};

} // namespace ECS
