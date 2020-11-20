/**
 * Based on code from entt v3.4.0
 * See https://github.com/skypjack/entt/blob/v3.4.0/src/entt/entity/registry.hpp
 * Made modifications to fully access `std::vector<variable_data> vars{};`
 * and perform thread-safety validations.
**/

#pragma once

#include "basis/ECS/ecs.hpp"

#include <entt/core/type_traits.hpp>

#include <base/macros.h>
#include <base/logging.h>
#include <base/location.h>
#include <base/sequenced_task_runner.h>
#include <base/threading/thread_checker.h>
#include <base/memory/weak_ptr.h>
#include <base/memory/ref_counted.h>
#include <base/memory/scoped_refptr.h>
#include <base/rvalue_cast.h>
#include <base/threading/thread_collision_warner.h>

#include <vector>
#include <string>
#include <atomic>

namespace base {

class SingleThreadTaskRunner;

template <typename T>
struct DefaultSingletonTraits;

} // namespace base

namespace ECS {

using idType = std::uint32_t;

// Stores vector of arbitrary typed objects,
// each object can be found by its type (using typeIndex).
/// \note API is not thread-safe
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class UnsafeTypeContext
{
 public:
  // Returns the sequential identifier of a given type.
  template<typename Type, typename = void>
  idType typeIndex() NO_EXCEPTION
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    // `static` because unique per each `Type`
    static const idType value = typeCounter_++;
    return value;
  }

  UnsafeTypeContext();

  ~UnsafeTypeContext();

  UnsafeTypeContext(
    UnsafeTypeContext&& other)
    : UnsafeTypeContext()
    {
      vars_ = base::rvalue_cast(other.vars_);

      typeCounter_ = base::rvalue_cast(other.typeCounter_);
    }

  // Move assignment operator
  //
  // MOTIVATION
  //
  // To use type as ECS component
  // it must be `move-constructible` and `move-assignable`
  UnsafeTypeContext& operator=(UnsafeTypeContext&& rhs)
  {
    if (this != &rhs)
    {
      vars_ = base::rvalue_cast(rhs.vars_);

      typeCounter_ = base::rvalue_cast(rhs.typeCounter_);
    }

    return *this;
  }

  struct variable_data {
    idType type_id;
    std::unique_ptr<void, void(*)(void *)> value;
#if DCHECK_IS_ON()
    std::string debug_name;
#endif // DCHECK_IS_ON()
  };

  /**
   * @brief Binds an object to the context of the registry.
   *
   * If the value already exists it is overwritten, otherwise a new instance
   * of the given type is created and initialized with the arguments provided.
   *
   * @tparam Type Type of object to set.
   * @tparam Args Types of arguments to use to construct the object.
   * @param args Parameters to use to initialize the value.
   * @return A reference to the newly created object.
   */
  template<typename Type, typename... Args>
  Type & set_var(const std::string& debug_name, Args &&... args)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DCHECK(!try_ctx_var<Type>());

    vars_.push_back(
      variable_data
      {
        typeIndex<Type>()
        , { new Type{std::forward<Args>(args)...}
            // custom deleter for `unique_ptr`
            , [](void *instance)
              {
                delete static_cast<Type*>(instance);
              }
          }
#if DCHECK_IS_ON()
        , debug_name
#endif // DCHECK_IS_ON()
      }
    );
#if !DCHECK_IS_ON()
    ignore_result(debug_name);
#endif // DCHECK_IS_ON()

    DVLOG(9)
      << "added to global context: "
      << debug_name
      << " with type_id: "
      << typeIndex<Type>();

#if DCHECK_IS_ON()
    for(const auto& var: vars_) {
      VLOG(9)
        << "(0) found global context var: "
        << var.debug_name
        << " with type_id: "
        << var.type_id;
    }
#endif // DCHECK_IS_ON()

    return *static_cast<Type*>(vars_.back().value.get());
  }

  /**
   * @brief Unsets a context variable if it exists.
   * @tparam Type Type of object to set.
   */
  template<typename Type>
  void unset_var(const base::Location& from_here)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    DVLOG(9)
      << from_here.ToString()
      << " removing from global context type index: "
      << typeIndex<Type>();

#if DCHECK_IS_ON()
    for(const auto& var: vars_) {
      VLOG(9)
        << "(1) found global context var: "
        << var.debug_name
        << " with type_id: "
        << var.type_id;
    }
#endif // DCHECK_IS_ON()

    vars_.erase(
      std::remove_if(
        vars_.begin()
        , vars_.end()
        , [this](auto &&var) {
#if DCHECK_IS_ON()
            if(var.type_id == typeIndex<Type>())
            {
              VLOG(9)
                << "removed from global context: "
                << var.debug_name
                << " with type_id: "
                << var.type_id
                << " and type index: "
                << typeIndex<Type>();
            }
#endif // DCHECK_IS_ON()
            return var.type_id == typeIndex<Type>();
          }
      )
      , vars_.end()
    );

#if DCHECK_IS_ON()
    for(const auto& var: vars_) {
      VLOG(9)
        << "(2) found global context var: "
        << var.debug_name
        << " with type_id: "
        << var.type_id;
    }
#endif // DCHECK_IS_ON()
  }

  /**
   * @brief Binds an object to the context of the registry.
   *
   * In case the context doesn't contain the given object, the parameters
   * provided are used to construct it.
   *
   * @tparam Type Type of object to set.
   * @tparam Args Types of arguments to use to construct the object.
   * @param args Parameters to use to initialize the object.
   * @return A reference to the object in the context of the registry.
   */
  template<typename Type, typename... Args>
  [[nodiscard]] /* don't ignore return value */
  Type & ctx_or_set_var(Args &&... args)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    auto *value = try_ctx_var<Type>();
    return value
      ? *value
      : set_var<Type>(std::forward<Args>(args)...);
  }

  /// \note works only if `Type` is `base::Optional<...>`
  /// because optional allows to re-create variable using same storage
  /// (i.e. using `placement new`)
  // Binds an object to the context of the registry.
  // If `Type` already exists it re-creates it using same storage
  // i.e. does NOT call `remove_if` and `vars_.push_back`.
  // Can be used to create `memory pool` where
  // unused data not freed instantly, but can be re-used again.
  template<typename Type, typename... Args>
  [[nodiscard]] /* don't ignore return value */
  Type & reset_or_create_var(
    const std::string debug_name
    , Args &&... args)
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    const bool useCache
      = try_ctx_var<Type>();

    DVLOG(99)
      << (useCache
          ? ("using preallocated " + debug_name)
          : ("allocating new " + debug_name));

    Type* channelCtx
      = &ctx_or_set_var<Type>(
          debug_name
          , base::in_place
          , std::forward<Args>(args)...);

    // If the value already exists it is overwritten
    if(useCache) {
      /// \note we do not call `set_var` for optimization purposes
      /// ( because `set_var` uses `remove_if` and `vars_.push_back`)
      /// i.e. use `base::Optional<...>` that uses placement new
      channelCtx->emplace(std::forward<Args>(args)...);
    }

    return *channelCtx;
  }

  /**
   * @brief Returns a pointer to an object in the context of the registry.
   * @tparam Type Type of object to get.
   * @return A pointer to the object if it exists in the context of the
   * registry, a null pointer otherwise.
   */
  template<typename Type>
  [[nodiscard]] /* don't ignore return value */
  Type* try_ctx_var()
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    auto it = std::find_if(
      vars_.cbegin()
      , vars_.cend()
      , [this](auto &&var)
        {
          return var.type_id == typeIndex<Type>();
        });
    return it == vars_.cend()
      ? nullptr
      : static_cast<Type*>(it->value.get());
  }

  /**
   * @brief Returns a reference to an object in the context of the registry.
   *
   * @warning
   * Attempting to get a context variable that doesn't exist results in
   * undefined behavior.<br/>
   * An assertion will abort the execution at runtime in debug mode in case of
   * invalid requests.
   *
   * @tparam Type Type of object to get.
   * @return A valid reference to the object in the context of the registry.
   */
  template<typename Type>
  [[nodiscard]] /* don't ignore return value */
  Type& ctx_var()
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    auto *instance = try_ctx_var<Type>();
    DCHECK(instance);
    return *instance;
  }

  /**
   * @brief Visits a registry and returns the types for its context variables.
   *
   * The signature of the function should be equivalent to the following:
   *
   * @code{.cpp}
   * void(const id_type);
   * @endcode
   *
   * Returned identifiers are those of the context variables currently set.
   *
   * @sa type_info
   *
   * @warning
   * It's not specified whether a context variable created during the visit is
   * returned or not to the caller.
   *
   * @tparam Func Type of the function object to invoke.
   * @param func A valid function object.
   */
  template<typename Func>
  void ctx_var(Func func) const
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);

    for(auto pos = vars_.size(); pos; --pos) {
      func(vars_[pos-1].type_id);
    }
  }

  std::vector<variable_data>& vars()
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return vars_;
  }

  const std::vector<variable_data>& vars() const
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return vars_;
  }

  size_t size() const
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return vars_.size();
  }

  bool empty() const
  {
    DFAKE_SCOPED_RECURSIVE_LOCK(debug_thread_collision_warner_);
    return vars_.empty();
  }

 private:
  // per-sequence counter for thread-safety reasons
  idType typeCounter_{};

  // Stores objects in the context of the registry.
  std::vector<variable_data> vars_{};

  // Thread collision warner to ensure that API is not called concurrently.
  // API allowed to call from multiple threads, but not
  // concurrently.
  DFAKE_MUTEX(debug_thread_collision_warner_);

  DISALLOW_COPY_AND_ASSIGN(UnsafeTypeContext);
};

} // namespace ECS
