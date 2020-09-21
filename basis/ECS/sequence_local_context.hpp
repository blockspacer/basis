/**
 * Based on code from entt v3.4.0
 * See https://github.com/skypjack/entt/blob/v3.4.0/src/entt/entity/registry.hpp
 * Made modifications to fully access `std::vector<variable_data> vars{};`
 * and perform thread-safety validations.
**/

#pragma once

#include "basis/ECS/ecs.hpp"
#include "basis/ECS/unsafe_context.hpp"

#include <entt/core/type_traits.hpp>

#include <base/macros.h>
#include <base/logging.h>
#include <base/location.h>
#include <base/sequenced_task_runner.h>
#include <base/threading/thread_checker.h>
#include <base/memory/weak_ptr.h>
#include <base/memory/ref_counted.h>
#include <base/memory/scoped_refptr.h>

#include <vector>
#include <string>
#include <atomic>

namespace base {

class SingleThreadTaskRunner;

template <typename T>
struct DefaultSingletonTraits;

} // namespace base

namespace ECS {

// Context that bound to single sequence.
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class SequenceLocalContext
  : public base::RefCountedThreadSafe<SequenceLocalContext>
{
 public:
  static base::WeakPtr<SequenceLocalContext>
    getSequenceLocalInstance(
      const base::Location& from_here
      // will be used in `DCHECK(task_runner.RunsTasksInCurrentSequence());`
      , scoped_refptr<base::SequencedTaskRunner> task_runner);

  UnsafeTypeContext& context()
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << FROM_HERE.ToString();

    return context_;
  }

  const UnsafeTypeContext& context() const
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << FROM_HERE.ToString();

    return context_;
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Component& ctx(
    const base::Location& from_here)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DCHECK(context_.try_ctx_var<Component>())
      << "failed SequenceLocalContext::ctx from "
      << from_here.ToString();

    return context_.ctx_var<Component>();
  }

  // usually context is NOT locked
  // during app creation or termination
  template<typename Component>
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  bool try_ctx(
    const base::Location& from_here)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

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
  [[nodiscard]] /* don't ignore return value */
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  Type& set_once(const base::Location& from_here
    , const std::string& debug_name
    , Args&&... args)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

    DVLOG(9)
      << "called SequenceLocalContext::set from "
      << from_here.ToString()
      << " added to global context: "
      << debug_name;

    /// \note can be set only once
    DCHECK(!context_.try_ctx_var<Type>());
    return context_.set_var<Type>(
      debug_name
      , std::forward<Args>(args)...);
  }

  template<typename Type>
  /// \note large `inline` functions cause Cache misses
  /// and affect efficiency negatively, so keep it small
  inline /* `inline` to eleminate function call overhead */
  void unset(const base::Location& from_here)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

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
  void try_unset(const base::Location& from_here)
  {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_)
      << "Unable to use global context from wrong thread "
      << from_here.ToString();

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

  base::WeakPtrFactory<SequenceLocalContext> weak_ptr_factory_{this};

  friend class base::RefCountedThreadSafe<SequenceLocalContext>;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SequenceLocalContext);
};

} // namespace ECS
