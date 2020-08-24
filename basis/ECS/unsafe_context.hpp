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

#include <vector>
#include <string>
#include <atomic>

namespace base {

class SingleThreadTaskRunner;

template <typename T>
struct DefaultSingletonTraits;

} // namespace base

namespace ECS {

// Stores vector of arbitrary typed objects,
// each object can be found by its type (using entt::type_info).
/// \note API is not thread-safe
// Inspired by entt context, see for details:
// https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system
class UnsafeTypeContext
{
 public:
  UnsafeTypeContext();

  ~UnsafeTypeContext();

  /*! @brief Alias declaration for type identifiers. */
  using id_type = ENTT_ID_TYPE;

  struct variable_data {
    id_type type_id;
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
    unset_var<Type>();
    vars.push_back(
      variable_data
      {
        entt::type_info<Type>::id()
        , { new Type{std::forward<Args>(args)...}
            , [](void *instance)
              {
                delete static_cast<Type *>(instance);
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
    return *static_cast<Type *>(vars.back().value.get());
  }

  /**
   * @brief Unsets a context variable if it exists.
   * @tparam Type Type of object to set.
   */
  template<typename Type>
  void unset_var()
  {
    vars.erase(
      std::remove_if(
        vars.begin()
        , vars.end()
        , [](auto &&var) {
#if DCHECK_IS_ON()
            if(var.type_id == entt::type_info<Type>::id())
            {
              VLOG(9)
                << "removed from global context:"
                << var.debug_name;
            }
#endif // DCHECK_IS_ON()
            return var.type_id == entt::type_info<Type>::id();
          }
      )
      , vars.end()
    );
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
    auto *value = try_ctx_var<Type>();
    return value
      ? *value
      : set_var<Type>(std::forward<Args>(args)...);
  }

  /**
   * @brief Returns a pointer to an object in the context of the registry.
   * @tparam Type Type of object to get.
   * @return A pointer to the object if it exists in the context of the
   * registry, a null pointer otherwise.
   */
  template<typename Type>
  [[nodiscard]] /* don't ignore return value */
  const Type * try_ctx_var() const
  {
    auto it = std::find_if(
      vars.cbegin()
      , vars.cend()
      , [](auto &&var)
        {
          return var.type_id == entt::type_info<Type>::id();
        });
    return it == vars.cend()
      ? nullptr
      : static_cast<const Type *>(it->value.get());
  }

  /*! @copydoc try_ctx */
  template<typename Type>
  [[nodiscard]] /* don't ignore return value */
  Type * try_ctx_var()
  {
    return const_cast<Type *>(
      std::as_const(*this).template try_ctx_var<Type>());
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
  const Type & ctx_var() const
  {
    const auto *instance = try_ctx_var<Type>();
    DCHECK(instance);
    return *instance;
  }

  /*! @copydoc ctx */
  template<typename Type>
  [[nodiscard]] /* don't ignore return value */
  Type & ctx_var()
  {
    return const_cast<Type &>(
      std::as_const(*this).template ctx_var<Type>());
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
    for(auto pos = vars.size(); pos; --pos) {
      func(vars[pos-1].type_id);
    }
  }

  std::vector<variable_data>& ref_vars()
  {
    return vars;
  }

  size_t size() const
  {
    return vars.size();
  }

  bool empty() const
  {
    return vars.empty();
  }

 private:
  // Stores objects in the context of the registry.
  std::vector<variable_data> vars{};

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(UnsafeTypeContext);
};

} // namespace ECS
