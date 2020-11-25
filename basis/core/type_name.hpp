#pragma once

#include <vector>
#include <string>

// MOTIVATION
//
// Maps some types with custom (provided by API user) string names.
// You can use it for debug puproses.
// (bool to "boolean", not "bool" i.e. no code generation possible here).
//
// USAGE
//
// // add custom type to `type_name()`
// DECLARE_TYPE_NAME(MyType, "my_type_is_super_cool")
//
// class CustomTypeNameGeneratorTag;
//
// // add same custom type to `type_name()`, but with other generator
// DECLARE_CUSTOM_TYPE_NAME(MyType, "my_type_is_super_cool", CustomTypeNameGeneratorTag)
//
namespace basis {

// Usage: `type_name<Type, DefaultTypeNameGeneratorTag>()`
class DefaultTypeNameGeneratorTag;

// the string representation of type name
/// \note You can create custom `NameGeneratorTag`
/// to prevent `type_name()` collision.
template<typename T, typename NameGeneratorTag = ::base::DefaultTypeNameGeneratorTag>
inline const char* type_name() {
  return "";
}

// macro to quickly declare traits information
#define DECLARE_CUSTOM_TYPE_NAME(Type, Name, Tag)     \
  template<>                                          \
  inline const char* type_name<Type, Tag>() {         \
    return Name;                                      \
  }

// macro to quickly declare traits information
#define DECLARE_TYPE_NAME(Type, Name)                 \
  DECLARE_CUSTOM_TYPE_NAME(Type, Name, ::base::DefaultTypeNameGeneratorTag)

DECLARE_TYPE_NAME(float, "float")

DECLARE_TYPE_NAME(double, "double")

DECLARE_TYPE_NAME(int, "int")

// int (non-negative)
DECLARE_TYPE_NAME(uint32_t, "uint32")

// long (non-negative)
DECLARE_TYPE_NAME(uint64_t, "uint64")

DECLARE_TYPE_NAME(std::string, "std::string")

DECLARE_TYPE_NAME(bool, "boolean")

}  // namespace basis
