#pragma once

#include "basis/status/status_macros.hpp"

#include <base/macros.h>
#include <base/callback_forward.h>

namespace basis {

using RepeatingStatusClosure = base::RepeatingCallback<basis::Status(void)>;

template<typename... Args>
using RepeatingStatus = base::RepeatingCallback<basis::Status(Args...)>;

template<typename T>
using RepeatingStatusOrClosure = base::RepeatingCallback<basis::StatusOr<T>(void)>;

template<typename T, typename... Args>
using RepeatingStatusOr = base::RepeatingCallback<basis::StatusOr<T>(Args...)>;

using OnceStatusClosure = base::OnceCallback<basis::Status(void)>;

template<typename... Args>
using OnceStatus = base::OnceCallback<basis::Status(Args...)>;

template<typename T>
using OnceStatusOrClosure = base::OnceCallback<basis::StatusOr<T>(void)>;

template<typename T, typename... Args>
using OnceStatusOr = base::OnceCallback<basis::StatusOr<T>(Args...)>;

}  // namespace basis
