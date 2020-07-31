#pragma once

#include <vector>
#include <string>

namespace base {

// the string representation of type name
template<typename T>
inline const char* type_name() {
  return "";
}

// macro to quickly declare traits information
#define DECLARE_TYPE_NAME(Type, Name)                 \
  template<>                                          \
  inline const char* type_name<Type>() {              \
    return Name;                                      \
  }

DECLARE_TYPE_NAME(float, "float")

DECLARE_TYPE_NAME(double, "double")

DECLARE_TYPE_NAME(int, "int")

// int (non-negative)
DECLARE_TYPE_NAME(uint32_t, "uint32")

// long (non-negative)
DECLARE_TYPE_NAME(uint64_t, "uint64")

DECLARE_TYPE_NAME(std::string, "std::string")

DECLARE_TYPE_NAME(bool, "boolean")

}  // namespace base
