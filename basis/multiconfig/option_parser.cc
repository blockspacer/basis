#include "basis/multiconfig/option_parser.hpp" // IWYU pragma: associated

#include "basis/status/status_macros.hpp"
#include "basis/core/type_name.hpp"
#include "basis/concept/dependent_false.hpp"

#include <base/gtest_prod_util.h>
#include <base/logging.h>
#include <base/bind.h>
#include <base/base_export.h>
#include <base/macros.h>
#include <base/optional.h>
#include <basic/rvalue_cast.h>
#include <base/strings/string_number_conversions.h>
#include <base/numerics/safe_conversions.h>
#include <base/numerics/floating_point_comparison.h>

#include <string>

namespace basis {

template<>
basis::StatusOr<std::string> parseOptionAs<std::string>(
  const std::string& str) NO_EXCEPTION
{
  return str;
}

/// \note Strings "TrUe", "True", "true" and "1" will result in `true` value.
template<>
basis::StatusOr<bool> parseOptionAs<bool>(
  const std::string& str) NO_EXCEPTION
{
  return base::EqualsCaseInsensitiveASCII(str, "true") || str == "1";
}

template<>
basis::StatusOr<int64_t> parseOptionAs<int64_t>(
  const std::string& str) NO_EXCEPTION
{
  int64_t output;
  RETURN_ERROR_IF(!base::StringToInt64(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<int64_t>()
    << " value: "
    << str;
  return output;
}

template<>
basis::StatusOr<int32_t> parseOptionAs<int32_t>(
  const std::string& str) NO_EXCEPTION
{
  int32_t output;
  RETURN_ERROR_IF(!base::StringToInt32(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<int32_t>()
    << " value: "
    << str;
  return output;
}

template<>
basis::StatusOr<uint32_t> parseOptionAs<uint32_t>(
  const std::string& str) NO_EXCEPTION
{
  uint32_t output;
  RETURN_ERROR_IF(!base::StringToUint32(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<uint32_t>()
    << " value: "
    << str;
  return output;
}

template<>
basis::StatusOr<uint64_t> parseOptionAs<uint64_t>(
  const std::string& str) NO_EXCEPTION
{
  uint64_t output;
  RETURN_ERROR_IF(!base::StringToUint64(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<uint64_t>()
    << " value: "
    << str;
  return output;
}

template<>
basis::StatusOr<double> parseOptionAs<double>(
  const std::string& str) NO_EXCEPTION
{
  double output;
  RETURN_ERROR_IF(!base::StringToDouble(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<double>()
    << " value: "
    << str;
  return output;
}

template<>
basis::StatusOr<float> parseOptionAs<float>(
  const std::string& str) NO_EXCEPTION
{
  double output;
  RETURN_ERROR_IF(!base::StringToDouble(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    << type_name<float>()
    << " value: "
    << str;
  DCHECK(base::WithinEpsilon(output, static_cast<double>(base::saturated_cast<float>(output))))
    << "unable to store " << output << " in float type";
  /// \note Converts from double with saturation
  /// to FLOAT_MAX, FLOAT_MIN, or 0 for NaN.
  return base::saturated_cast<float>(output);
}

// redefinition
#if 0
template<>
basis::StatusOr<size_t> parseOptionAs<size_t>(
  const std::string& str) NO_EXCEPTION
{
  size_t output;
  RETURN_ERROR_IF(!base::StringToSizeT(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    //<< type_name<size_t>()
    << " value: "
    << str;
  return output;
}
#endif

// redefinition
#if 0
template<>
basis::StatusOr<int> parseOptionAs<int>(
  const std::string& str) NO_EXCEPTION
{
  int output;
  RETURN_ERROR_IF(!base::StringToInt(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    //<< type_name<int>()
    << " value: "
    << str;
  return output;
}
#endif

// redefinition
#if 0
template<>
basis::StatusOr<unsigned> parseOptionAs<unsigned>(
  const std::string& str) NO_EXCEPTION
{
  unsigned output;
  RETURN_ERROR_IF(!base::StringToUint(str, &output))
    << "Configuration value expected to be valid."
    << " Can not parse to "
    //<< type_name<unsigned>()
    << " value: "
    << str;
  return output;
}
#endif

} // namespace basis
