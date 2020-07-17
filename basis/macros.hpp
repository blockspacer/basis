#pragma once

#define DEFAULT_CONSTRUCTOR(ClassName) \
  ClassName() = default;

#define DEFAULT_COPY(ClassName)                                 \
  ClassName(const ClassName &rhs) = default;                    \
  ClassName &operator=(const ClassName &rhs) = default;         \
  /* Fails during compile-time if default-copy can not work. */ \
  static void COPYABLE_DEMAND_COPY_CAN_COMPILE() {              \
    (void) static_cast<ClassName& (ClassName::*)(               \
        const ClassName&)>(&ClassName::operator=);              \
  }

#define DEFAULT_MOVE(ClassName)                             \
  ClassName(ClassName &&rhs) = default;            \
  ClassName &operator=(ClassName &&rhs) = default;

#define DEFAULT_COPY_AND_MOVE(ClassName)                \
  DEFAULT_COPY(ClassName)                               \
  DEFAULT_MOVE(ClassName)
