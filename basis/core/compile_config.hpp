//
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This header file defines a set of macros for checking the presence of
// important compiler and platform features. Such macros can be used to
// produce portable code by parameterizing compilation based on the presence or
// lack of a given feature.
//
// We define a "feature" as some interface we wish to program to: for example,
// a library function or system call. A value of `1` indicates support for
// that feature; any other value indicates the feature support is undefined.
//
// Example:
//
// Suppose a programmer wants to write a program that uses the 'mmap()' system
// call. The Abseil macro for that feature (`BASIS_HAVE_MMAP`) allows you to
// selectively include the `mmap.h` header and bracket code using that feature
// in the macro:
//
//   #include "BASIS/base/config.h"
//
//   #ifdef BASIS_HAVE_MMAP
//   #include "sys/mman.h"
//   #endif  //BASIS_HAVE_MMAP
//
//   ...
//   #ifdef BASIS_HAVE_MMAP
//   void *ptr = mmap(...);
//   ...
//   #endif  // BASIS_HAVE_MMAP

#pragma once

// Included for the __GLIBC__ macro (or similar macros on other systems).
#include <limits.h>

#ifdef __cplusplus
// Included for __GLIBCXX__, _LIBCPP_VERSION
#include <cstddef>
#endif  // __cplusplus

#if defined(__APPLE__)
// Included for TARGET_OS_IPHONE, __IPHONE_OS_VERSION_MIN_REQUIRED,
// __IPHONE_8_0.
#include <Availability.h>
#include <TargetConditionals.h>
#endif

#include "basis/options.hpp"
#include "basis/policy_checks.hpp"

// Helper macro to convert a CPP variable to a string literal.
#define BASIS_INTERNAL_DO_TOKEN_STR(x) #x
#define BASIS_INTERNAL_TOKEN_STR(x) BASIS_INTERNAL_DO_TOKEN_STR(x)

// -----------------------------------------------------------------------------
// Abseil namespace annotations
// -----------------------------------------------------------------------------

// BASIS_NAMESPACE_BEGIN/BASIS_NAMESPACE_END
//
// An annotation placed at the beginning/end of each `namespace BASIS` scope.
// This is used to inject an inline namespace.
//
// The proper way to write Abseil code in the `BASIS` namespace is:
//
// namespace BASIS {
// BASIS_NAMESPACE_BEGIN
//
// void Foo();  // BASIS::Foo().
//
// BASIS_NAMESPACE_END
// }  // namespace BASIS
//
// Users of Abseil should not use these macros, because users of Abseil should
// not write `namespace BASIS {` in their own code for any reason.  (Abseil does
// not support forward declarations of its own types, nor does it support
// user-provided specialization of Abseil templates.  Code that violates these
// rules may be broken without warning.)
#if !defined(BASIS_OPTION_USE_INLINE_NAMESPACE) || \
    !defined(BASIS_OPTION_INLINE_NAMESPACE_NAME)
#error options.h is misconfigured.
#endif

// Check that BASIS_OPTION_INLINE_NAMESPACE_NAME is neither "head" nor ""
#if defined(__cplusplus) && BASIS_OPTION_USE_INLINE_NAMESPACE == 1

#define BASIS_INTERNAL_INLINE_NAMESPACE_STR \
  BASIS_INTERNAL_TOKEN_STR(BASIS_OPTION_INLINE_NAMESPACE_NAME)

static_assert(BASIS_INTERNAL_INLINE_NAMESPACE_STR[0] != '\0',
              "options.h misconfigured: BASIS_OPTION_INLINE_NAMESPACE_NAME must "
              "not be empty.");
static_assert(BASIS_INTERNAL_INLINE_NAMESPACE_STR[0] != 'h' ||
                  BASIS_INTERNAL_INLINE_NAMESPACE_STR[1] != 'e' ||
                  BASIS_INTERNAL_INLINE_NAMESPACE_STR[2] != 'a' ||
                  BASIS_INTERNAL_INLINE_NAMESPACE_STR[3] != 'd' ||
                  BASIS_INTERNAL_INLINE_NAMESPACE_STR[4] != '\0',
              "options.h misconfigured: BASIS_OPTION_INLINE_NAMESPACE_NAME must "
              "be changed to a new, unique identifier name.");

#endif

#if BASIS_OPTION_USE_INLINE_NAMESPACE == 0
#define BASIS_NAMESPACE_BEGIN
#define BASIS_NAMESPACE_END
#elif BASIS_OPTION_USE_INLINE_NAMESPACE == 1
#define BASIS_NAMESPACE_BEGIN \
  inline namespace BASIS_OPTION_INLINE_NAMESPACE_NAME {
#define BASIS_NAMESPACE_END }
#else
#error options.h is misconfigured.
#endif

// -----------------------------------------------------------------------------
// Compiler Feature Checks
// -----------------------------------------------------------------------------

// BASIS_HAVE_BUILTIN()
//
// Checks whether the compiler supports a Clang Feature Checking Macro, and if
// so, checks whether it supports the provided builtin function "x" where x
// is one of the functions noted in
// https://clang.llvm.org/docs/LanguageExtensions.html
//
// Note: Use this macro to avoid an extra level of #ifdef __has_builtin check.
// http://releases.llvm.org/3.3/tools/clang/docs/LanguageExtensions.html
#ifdef __has_builtin
#define BASIS_HAVE_BUILTIN(x) __has_builtin(x)
#else
#define BASIS_HAVE_BUILTIN(x) 0
#endif

#if defined(__is_identifier)
#define BASIS_INTERNAL_HAS_KEYWORD(x) !(__is_identifier(x))
#else
#define BASIS_INTERNAL_HAS_KEYWORD(x) 0
#endif

// BASIS_HAVE_TLS is defined to 1 when __thread should be supported.
// We assume __thread is supported on Linux when compiled with Clang or compiled
// against libstdc++ with _GLIBCXX_HAVE_TLS defined.
#ifdef BASIS_HAVE_TLS
#error BASIS_HAVE_TLS cannot be directly set
#elif defined(__linux__) && (defined(__clang__) || defined(_GLIBCXX_HAVE_TLS))
#define BASIS_HAVE_TLS 1
#endif

// BASIS_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
//
// Checks whether `std::is_trivially_destructible<T>` is supported.
//
// Notes: All supported compilers using libc++ support this feature, as does
// gcc >= 4.8.1 using libstdc++, and Visual Studio.
#ifdef BASIS_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
#error BASIS_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE cannot be directly set
#elif defined(_LIBCPP_VERSION) ||                                        \
    (!defined(__clang__) && defined(__GNUC__) && defined(__GLIBCXX__) && \
     (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) ||        \
    defined(_MSC_VER)
#define BASIS_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE 1
#endif

// BASIS_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE
//
// Checks whether `std::is_trivially_default_constructible<T>` and
// `std::is_trivially_copy_constructible<T>` are supported.

// BASIS_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE
//
// Checks whether `std::is_trivially_copy_assignable<T>` is supported.

// Notes: Clang with libc++ supports these features, as does gcc >= 5.1 with
// either libc++ or libstdc++, and Visual Studio (but not NVCC).
#if defined(BASIS_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE)
#error BASIS_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE cannot be directly set
#elif defined(BASIS_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE)
#error BASIS_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE cannot directly set
#elif (defined(__clang__) && defined(_LIBCPP_VERSION)) ||        \
    (!defined(__clang__) && defined(__GNUC__) &&                 \
     (__GNUC__ > 7 || (__GNUC__ == 7 && __GNUC_MINOR__ >= 4)) && \
     (defined(_LIBCPP_VERSION) || defined(__GLIBCXX__))) ||      \
    (defined(_MSC_VER) && !defined(__NVCC__))
#define BASIS_HAVE_STD_IS_TRIVIALLY_CONSTRUCTIBLE 1
#define BASIS_HAVE_STD_IS_TRIVIALLY_ASSIGNABLE 1
#endif

// BASIS_HAVE_SOURCE_LOCATION_CURRENT
//
// Indicates whether `BASIS::SourceLocation::current()` will return useful
// information in some contexts.
#ifndef BASIS_HAVE_SOURCE_LOCATION_CURRENT
#if BASIS_INTERNAL_HAS_KEYWORD(__builtin_LINE) && \
    BASIS_INTERNAL_HAS_KEYWORD(__builtin_FILE)
#define BASIS_HAVE_SOURCE_LOCATION_CURRENT 1
#endif
#endif

// BASIS_HAVE_THREAD_LOCAL
//
// Checks whether C++11's `thread_local` storage duration specifier is
// supported.
#ifdef BASIS_HAVE_THREAD_LOCAL
#error BASIS_HAVE_THREAD_LOCAL cannot be directly set
#elif defined(__APPLE__)
// Notes:
// * Xcode's clang did not support `thread_local` until version 8, and
//   even then not for all iOS < 9.0.
// * Xcode 9.3 started disallowing `thread_local` for 32-bit iOS simulator
//   targeting iOS 9.x.
// * Xcode 10 moves the deployment target check for iOS < 9.0 to link time
//   making __has_feature unreliable there.
//
// Otherwise, `__has_feature` is only supported by Clang so it has be inside
// `defined(__APPLE__)` check.
#if __has_feature(cxx_thread_local) && \
    !(TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_9_0)
#define BASIS_HAVE_THREAD_LOCAL 1
#endif
#else  // !defined(__APPLE__)
#define BASIS_HAVE_THREAD_LOCAL 1
#endif

// There are platforms for which TLS should not be used even though the compiler
// makes it seem like it's supported (Android NDK < r12b for example).
// This is primarily because of linker problems and toolchain misconfiguration:
// Abseil does not intend to support this indefinitely. Currently, the newest
// toolchain that we intend to support that requires this behavior is the
// r11 NDK - allowing for a 5 year support window on that means this option
// is likely to be removed around June of 2021.
// TLS isn't supported until NDK r12b per
// https://developer.android.com/ndk/downloads/revision_history.html
// Since NDK r16, `__NDK_MAJOR__` and `__NDK_MINOR__` are defined in
// <android/ndk-version.h>. For NDK < r16, users should define these macros,
// e.g. `-D__NDK_MAJOR__=11 -D__NKD_MINOR__=0` for NDK r11.
#if defined(__ANDROID__) && defined(__clang__)
#if __has_include(<android/ndk-version.h>)
#include <android/ndk-version.h>
#endif  // __has_include(<android/ndk-version.h>)
#if defined(__ANDROID__) && defined(__clang__) && defined(__NDK_MAJOR__) && \
    defined(__NDK_MINOR__) &&                                               \
    ((__NDK_MAJOR__ < 12) || ((__NDK_MAJOR__ == 12) && (__NDK_MINOR__ < 1)))
#undef BASIS_HAVE_TLS
#undef BASIS_HAVE_THREAD_LOCAL
#endif
#endif  // defined(__ANDROID__) && defined(__clang__)

// BASIS_HAVE_INTRINSIC_INT128
//
// Checks whether the __int128 compiler extension for a 128-bit integral type is
// supported.
//
// Note: __SIZEOF_INT128__ is defined by Clang and GCC when __int128 is
// supported, but we avoid using it in certain cases:
// * On Clang:
//   * Building using Clang for Windows, where the Clang runtime library has
//     128-bit support only on LP64 architectures, but Windows is LLP64.
// * On Nvidia's nvcc:
//   * nvcc also defines __GNUC__ and __SIZEOF_INT128__, but not all versions
//     actually support __int128.
#ifdef BASIS_HAVE_INTRINSIC_INT128
#error BASIS_HAVE_INTRINSIC_INT128 cannot be directly set
#elif defined(__SIZEOF_INT128__)
#if (defined(__clang__) && !defined(_WIN32)) || \
    (defined(__CUDACC__) && __CUDACC_VER_MAJOR__ >= 9) ||                \
    (defined(__GNUC__) && !defined(__clang__) && !defined(__CUDACC__))
#define BASIS_HAVE_INTRINSIC_INT128 1
#elif defined(__CUDACC__)
// __CUDACC_VER__ is a full version number before CUDA 9, and is defined to a
// string explaining that it has been removed starting with CUDA 9. We use
// nested #ifs because there is no short-circuiting in the preprocessor.
// NOTE: `__CUDACC__` could be undefined while `__CUDACC_VER__` is defined.
#if __CUDACC_VER__ >= 70000
#define BASIS_HAVE_INTRINSIC_INT128 1
#endif  // __CUDACC_VER__ >= 70000
#endif  // defined(__CUDACC__)
#endif  // BASIS_HAVE_INTRINSIC_INT128

// BASIS_HAVE_EXCEPTIONS
//
// Checks whether the compiler both supports and enables exceptions. Many
// compilers support a "no exceptions" mode that disables exceptions.
//
// Generally, when BASIS_HAVE_EXCEPTIONS is not defined:
//
// * Code using `throw` and `try` may not compile.
// * The `noexcept` specifier will still compile and behave as normal.
// * The `noexcept` operator may still return `false`.
//
// For further details, consult the compiler's documentation.
#ifdef BASIS_HAVE_EXCEPTIONS
#error BASIS_HAVE_EXCEPTIONS cannot be directly set.

#elif defined(__clang__)

#if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 6)
// Clang >= 3.6
#if __has_feature(cxx_exceptions)
#define BASIS_HAVE_EXCEPTIONS 1
#endif  // __has_feature(cxx_exceptions)
#else
// Clang < 3.6
// http://releases.llvm.org/3.6.0/tools/clang/docs/ReleaseNotes.html#the-exceptions-macro
#if defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)
#define BASIS_HAVE_EXCEPTIONS 1
#endif  // defined(__EXCEPTIONS) && __has_feature(cxx_exceptions)
#endif  // __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 6)

// Handle remaining special cases and default to exceptions being supported.
#elif !(defined(__GNUC__) && (__GNUC__ < 5) && !defined(__EXCEPTIONS)) &&    \
    !(defined(__GNUC__) && (__GNUC__ >= 5) && !defined(__cpp_exceptions)) && \
    !(defined(_MSC_VER) && !defined(_CPPUNWIND))
#define BASIS_HAVE_EXCEPTIONS 1
#endif

// -----------------------------------------------------------------------------
// Platform Feature Checks
// -----------------------------------------------------------------------------

// Currently supported operating systems and associated preprocessor
// symbols:
//
//   Linux and Linux-derived           __linux__
//   Android                           __ANDROID__ (implies __linux__)
//   Linux (non-Android)               __linux__ && !__ANDROID__
//   Darwin (macOS and iOS)            __APPLE__
//   Akaros (http://akaros.org)        __ros__
//   Windows                           _WIN32
//   NaCL                              __native_client__
//   AsmJS                             __asmjs__
//   WebAssembly                       __wasm__
//   Fuchsia                           __Fuchsia__
//
// Note that since Android defines both __ANDROID__ and __linux__, one
// may probe for either Linux or Android by simply testing for __linux__.

// BASIS_HAVE_MMAP
//
// Checks whether the platform has an mmap(2) implementation as defined in
// POSIX.1-2001.
#ifdef BASIS_HAVE_MMAP
#error BASIS_HAVE_MMAP cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||   \
    defined(__ros__) || defined(__native_client__) || defined(__asmjs__) || \
    defined(__wasm__) || defined(__Fuchsia__) || defined(__sun) || \
    defined(__ASYLO__)
#define BASIS_HAVE_MMAP 1
#endif

// BASIS_HAVE_PTHREAD_GETSCHEDPARAM
//
// Checks whether the platform implements the pthread_(get|set)schedparam(3)
// functions as defined in POSIX.1-2001.
#ifdef BASIS_HAVE_PTHREAD_GETSCHEDPARAM
#error BASIS_HAVE_PTHREAD_GETSCHEDPARAM cannot be directly set
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__ros__)
#define BASIS_HAVE_PTHREAD_GETSCHEDPARAM 1
#endif

// BASIS_HAVE_SCHED_YIELD
//
// Checks whether the platform implements sched_yield(2) as defined in
// POSIX.1-2001.
#ifdef BASIS_HAVE_SCHED_YIELD
#error BASIS_HAVE_SCHED_YIELD cannot be directly set
#elif defined(__linux__) || defined(__ros__) || defined(__native_client__)
#define BASIS_HAVE_SCHED_YIELD 1
#endif

// BASIS_HAVE_SEMAPHORE_H
//
// Checks whether the platform supports the <semaphore.h> header and sem_init(3)
// family of functions as standardized in POSIX.1-2001.
//
// Note: While Apple provides <semaphore.h> for both iOS and macOS, it is
// explicitly deprecated and will cause build failures if enabled for those
// platforms.  We side-step the issue by not defining it here for Apple
// platforms.
#ifdef BASIS_HAVE_SEMAPHORE_H
#error BASIS_HAVE_SEMAPHORE_H cannot be directly set
#elif defined(__linux__) || defined(__ros__)
#define BASIS_HAVE_SEMAPHORE_H 1
#endif

// BASIS_HAVE_ALARM
//
// Checks whether the platform supports the <signal.h> header and alarm(2)
// function as standardized in POSIX.1-2001.
#ifdef BASIS_HAVE_ALARM
#error BASIS_HAVE_ALARM cannot be directly set
#elif defined(__GOOGLE_GRTE_VERSION__)
// feature tests for Google's GRTE
#define BASIS_HAVE_ALARM 1
#elif defined(__GLIBC__)
// feature test for glibc
#define BASIS_HAVE_ALARM 1
#elif defined(_MSC_VER)
// feature tests for Microsoft's library
#elif defined(__MINGW32__)
// mingw32 doesn't provide alarm(2):
// https://osdn.net/projects/mingw/scm/git/mingw-org-wsl/blobs/5.2-trunk/mingwrt/include/unistd.h
// mingw-w64 provides a no-op implementation:
// https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-crt/misc/alarm.c
#elif defined(__EMSCRIPTEN__)
// emscripten doesn't support signals
#elif defined(__Fuchsia__)
// Signals don't exist on fuchsia.
#elif defined(__native_client__)
#else
// other standard libraries
#define BASIS_HAVE_ALARM 1
#endif

// BASIS_IS_LITTLE_ENDIAN
// BASIS_IS_BIG_ENDIAN
//
// Checks the endianness of the platform.
//
// Notes: uses the built in endian macros provided by GCC (since 4.6) and
// Clang (since 3.2); see
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html.
// Otherwise, if _WIN32, assume little endian. Otherwise, bail with an error.
#if defined(BASIS_IS_BIG_ENDIAN)
#error "BASIS_IS_BIG_ENDIAN cannot be directly set."
#endif
#if defined(BASIS_IS_LITTLE_ENDIAN)
#error "BASIS_IS_LITTLE_ENDIAN cannot be directly set."
#endif

#if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
     __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BASIS_IS_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BASIS_IS_BIG_ENDIAN 1
#elif defined(_WIN32)
#define BASIS_IS_LITTLE_ENDIAN 1
#else
#error "BASIS endian detection needs to be set up for your compiler"
#endif

// macOS 10.13 and iOS 10.11 don't let you use <any>, <optional>, or <variant>
// even though the headers exist and are publicly noted to work.  See
// https://github.com/abseil/abseil-cpp/issues/207 and
// https://developer.apple.com/documentation/xcode_release_notes/xcode_10_release_notes
// libc++ spells out the availability requirements in the file
// llvm-project/libcxx/include/__config via the #define
// _LIBCPP_AVAILABILITY_BAD_OPTIONAL_ACCESS.
#if defined(__APPLE__) && defined(_LIBCPP_VERSION) && \
  ((defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101400) || \
  (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ < 120000) || \
  (defined(__ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_WATCH_OS_VERSION_MIN_REQUIRED__ < 120000) || \
  (defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__) && \
   __ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__ < 50000))
#define BASIS_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 1
#else
#define BASIS_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE 0
#endif

// BASIS_HAVE_STD_ANY
//
// Checks whether C++17 std::any is available by checking whether <any> exists.
#ifdef BASIS_HAVE_STD_ANY
#error "BASIS_HAVE_STD_ANY cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<any>) && __cplusplus >= 201703L && \
    !BASIS_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define BASIS_HAVE_STD_ANY 1
#endif
#endif

// BASIS_HAVE_STD_OPTIONAL
//
// Checks whether C++17 std::optional is available.
#ifdef BASIS_HAVE_STD_OPTIONAL
#error "BASIS_HAVE_STD_OPTIONAL cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<optional>) && __cplusplus >= 201703L && \
    !BASIS_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define BASIS_HAVE_STD_OPTIONAL 1
#endif
#endif

// BASIS_HAVE_STD_VARIANT
//
// Checks whether C++17 std::variant is available.
#ifdef BASIS_HAVE_STD_VARIANT
#error "BASIS_HAVE_STD_VARIANT cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<variant>) && __cplusplus >= 201703L && \
    !BASIS_INTERNAL_APPLE_CXX17_TYPES_UNAVAILABLE
#define BASIS_HAVE_STD_VARIANT 1
#endif
#endif

// BASIS_HAVE_STD_STRING_VIEW
//
// Checks whether C++17 std::string_view is available.
#ifdef BASIS_HAVE_STD_STRING_VIEW
#error "BASIS_HAVE_STD_STRING_VIEW cannot be directly set."
#endif

#ifdef __has_include
#if __has_include(<string_view>) && __cplusplus >= 201703L
#define BASIS_HAVE_STD_STRING_VIEW 1
#endif
#endif

// For MSVC, `__has_include` is supported in VS 2017 15.3, which is later than
// the support for <optional>, <any>, <string_view>, <variant>. So we use
// _MSC_VER to check whether we have VS 2017 RTM (when <optional>, <any>,
// <string_view>, <variant> is implemented) or higher. Also, `__cplusplus` is
// not correctly set by MSVC, so we use `_MSVC_LANG` to check the language
// version.
// TODO(zhangxy): fix tests before enabling aliasing for `std::any`.
#if defined(_MSC_VER) && _MSC_VER >= 1910 && \
    ((defined(_MSVC_LANG) && _MSVC_LANG > 201402) || __cplusplus > 201402)
// #define BASIS_HAVE_STD_ANY 1
#define BASIS_HAVE_STD_OPTIONAL 1
#define BASIS_HAVE_STD_VARIANT 1
#define BASIS_HAVE_STD_STRING_VIEW 1
#endif

// BASIS_USES_STD_ANY
//
// Indicates whether BASIS::any is an alias for std::any.
#if !defined(BASIS_OPTION_USE_STD_ANY)
#error options.h is misconfigured.
#elif BASIS_OPTION_USE_STD_ANY == 0 || \
    (BASIS_OPTION_USE_STD_ANY == 2 && !defined(BASIS_HAVE_STD_ANY))
#undef BASIS_USES_STD_ANY
#elif BASIS_OPTION_USE_STD_ANY == 1 || \
    (BASIS_OPTION_USE_STD_ANY == 2 && defined(BASIS_HAVE_STD_ANY))
#define BASIS_USES_STD_ANY 1
#else
#error options.h is misconfigured.
#endif

// BASIS_USES_STD_OPTIONAL
//
// Indicates whether BASIS::optional is an alias for std::optional.
#if !defined(BASIS_OPTION_USE_STD_OPTIONAL)
#error options.h is misconfigured.
#elif BASIS_OPTION_USE_STD_OPTIONAL == 0 || \
    (BASIS_OPTION_USE_STD_OPTIONAL == 2 && !defined(BASIS_HAVE_STD_OPTIONAL))
#undef BASIS_USES_STD_OPTIONAL
#elif BASIS_OPTION_USE_STD_OPTIONAL == 1 || \
    (BASIS_OPTION_USE_STD_OPTIONAL == 2 && defined(BASIS_HAVE_STD_OPTIONAL))
#define BASIS_USES_STD_OPTIONAL 1
#else
#error options.h is misconfigured.
#endif

// BASIS_USES_STD_VARIANT
//
// Indicates whether BASIS::variant is an alias for std::variant.
#if !defined(BASIS_OPTION_USE_STD_VARIANT)
#error options.h is misconfigured.
#elif BASIS_OPTION_USE_STD_VARIANT == 0 || \
    (BASIS_OPTION_USE_STD_VARIANT == 2 && !defined(BASIS_HAVE_STD_VARIANT))
#undef BASIS_USES_STD_VARIANT
#elif BASIS_OPTION_USE_STD_VARIANT == 1 || \
    (BASIS_OPTION_USE_STD_VARIANT == 2 && defined(BASIS_HAVE_STD_VARIANT))
#define BASIS_USES_STD_VARIANT 1
#else
#error options.h is misconfigured.
#endif

// BASIS_USES_STD_STRING_VIEW
//
// Indicates whether BASIS::string_view is an alias for std::string_view.
#if !defined(BASIS_OPTION_USE_STD_STRING_VIEW)
#error options.h is misconfigured.
#elif BASIS_OPTION_USE_STD_STRING_VIEW == 0 || \
    (BASIS_OPTION_USE_STD_STRING_VIEW == 2 &&  \
     !defined(BASIS_HAVE_STD_STRING_VIEW))
#undef BASIS_USES_STD_STRING_VIEW
#elif BASIS_OPTION_USE_STD_STRING_VIEW == 1 || \
    (BASIS_OPTION_USE_STD_STRING_VIEW == 2 &&  \
     defined(BASIS_HAVE_STD_STRING_VIEW))
#define BASIS_USES_STD_STRING_VIEW 1
#else
#error options.h is misconfigured.
#endif

// In debug mode, MSVC 2017's std::variant throws a EXCEPTION_ACCESS_VIOLATION
// SEH exception from emplace for variant<SomeStruct> when constructing the
// struct can throw. This defeats some of variant_test and
// variant_exception_safety_test.
#if defined(_MSC_VER) && _MSC_VER >= 1700 && defined(_DEBUG)
#define BASIS_INTERNAL_MSVC_2017_DBG_MODE
#endif

// BASIS_INTERNAL_MANGLED_NS
// BASIS_INTERNAL_MANGLED_BACKREFERENCE
//
// Internal macros for building up mangled names in our internal fork of CCTZ.
// This implementation detail is only needed and provided for the MSVC build.
//
// These macros both expand to string literals.  BASIS_INTERNAL_MANGLED_NS is
// the mangled spelling of the `BASIS` namespace, and
// BASIS_INTERNAL_MANGLED_BACKREFERENCE is a back-reference integer representing
// the proper count to skip past the CCTZ fork namespace names.  (This number
// is one larger when there is an inline namespace name to skip.)
#if defined(_MSC_VER)
#if BASIS_OPTION_USE_INLINE_NAMESPACE == 0
#define BASIS_INTERNAL_MANGLED_NS "BASIS"
#define BASIS_INTERNAL_MANGLED_BACKREFERENCE "5"
#else
#define BASIS_INTERNAL_MANGLED_NS \
  BASIS_INTERNAL_TOKEN_STR(BASIS_OPTION_INLINE_NAMESPACE_NAME) "@BASIS"
#define BASIS_INTERNAL_MANGLED_BACKREFERENCE "6"
#endif
#endif

#undef BASIS_INTERNAL_HAS_KEYWORD

// BASIS_DLL
//
// When building Abseil as a DLL, this macro expands to `__declspec(dllexport)`
// so we can annotate symbols appropriately as being exported. When used in
// headers consuming a DLL, this macro expands to `__declspec(dllimport)` so
// that consumers know the symbol is defined inside the DLL. In all other cases,
// the macro expands to nothing.
#if defined(_MSC_VER)
#if defined(BASIS_BUILD_DLL)
#define BASIS_DLL __declspec(dllexport)
#elif defined(BASIS_CONSUME_DLL)
#define BASIS_DLL __declspec(dllimport)
#else
#define BASIS_DLL
#endif
#else
#define BASIS_DLL
#endif  // defined(_MSC_VER)
