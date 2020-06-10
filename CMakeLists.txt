# see https://github.com/chromium/chromium/blob/9db0b5162b8648833c3a6b1af044cebd9d7d3855/base/BUILD.gn

cmake_minimum_required(VERSION 3.4)

# project() must be before checks, see https://stackoverflow.com/a/26437667/10904212
set(PROJECT_NAME "basis")
set(PROJECT_VERSION 1.0.0.1)
project(${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  LANGUAGES CXX C)

# Hack to depend NOT on CONAN_PKG::libname, but on libname directly
# See for details
# https://docs.conan.io/en/latest/developing_packages/workspaces.html
set(${PROJECT_NAME}_LOCAL_BUILD FALSE CACHE BOOL "${PROJECT_NAME}_LOCAL_BUILD")
message(STATUS "${PROJECT_NAME}_LOCAL_BUILD=${${PROJECT_NAME}_LOCAL_BUILD}")

set(STATIC_LIB_NAME ${PROJECT_NAME})

option(ENABLE_TESTS "Enable tests" OFF)

option(ENABLE_VALGRIND
  "Enable valgrind" OFF)

# TODO: __do_global_dtors_aux, base::debug::CollectStackTrace
option(ENABLE_VALGRIND_TESTS
  "Enable valgrind for unit tests" OFF)

# see https://github.com/Ericsson/codechecker/blob/master/tools/report-converter/README.md#undefined-behaviour-sanitizer
# NOTE: Compile with -g and -fno-omit-frame-pointer
# to get proper debug information in your binary.
# NOTE: Run your program with environment variable UBSAN_OPTIONS=print_stacktrace=1.
# see https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
option(ENABLE_UBSAN
  "Enable Undefined Behaviour Sanitizer" OFF)

# see https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer
# see https://github.com/Ericsson/codechecker/blob/master/tools/report-converter/README.md#address-sanitizer
# NOTE: Compile with -g and -fno-omit-frame-pointer
# to get proper debug information in your binary.
# NOTE: use ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=suppr.txt
# NOTE: You need the ASAN_OPTIONS=symbolize=1
# to turn on resolving addresses in object code
# to source code line numbers and filenames.
# This option is implicit for Clang but it won't do any harm.
# see https://github.com/google/sanitizers/wiki/SanitizerCommonFlags
option(ENABLE_ASAN
  "Enable Address Sanitizer" OFF)

# see https://github.com/Ericsson/codechecker/blob/master/tools/report-converter/README.md#memory-sanitizer
# NOTE: Compile with -g and -fno-omit-frame-pointer
# to get proper debug information in your binary.
option(ENABLE_MSAN
  "Enable Memory Sanitizer" OFF)

# see https://github.com/Ericsson/codechecker/blob/master/tools/report-converter/README.md#thread-sanitizer
# NOTE: Compile with -g
# to get proper debug information in your binary.
option(ENABLE_TSAN
  "Enable Thread Sanitizer" OFF)

# Hack to depend NOT on CONAN_PKG::libname, but on libname directly
# See for details
# https://docs.conan.io/en/latest/developing_packages/workspaces.html
set(LOCAL_BUILD FALSE CACHE BOOL "LOCAL_BUILD")
message(STATUS "LOCAL_BUILD=${LOCAL_BUILD}")

# --------------------------- conan configuration ------------------------------

# Note: FetchContent is available since CMake 3.11
# see https://cmake.org/cmake/help/git-master/module/FetchContent.html
# By default, everything is downloaded into your build directory
# Once CMake successfully downloads our external content, it sets two variables that can be used in CMakeLists.txt to locate the new data:
# 1 <resource_name>_SOURCE_DIR
#   specifies the location of the downloaded sources,
# 2 <resource_name>_BINARY_DIR
#   specifies where is the default build directory for the downloaded sources.
include(FetchContent)

set(FetchContent_cmake_utils "cmake_utils")
FetchContent_Declare(
  ${FetchContent_cmake_utils}
  PREFIX external_dependencies/${FetchContent_cmake_utils}
  GIT_REPOSITORY https://github.com/blockspacer/cmake_utils.git
  GIT_TAG origin/master
  # Disable warning about detached HEAD https://stackoverflow.com/a/36794768
  GIT_CONFIG        advice.detachedHead=false
  SOURCE_DIR        "${CMAKE_BINARY_DIR}/${FetchContent_cmake_utils}"
  BINARY_DIR        "${CMAKE_BINARY_DIR}/${FetchContent_cmake_utils}-build"
  CMAKE_ARGS        "-Wno-dev"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
FetchContent_GetProperties(${FetchContent_cmake_utils})
if (NOT ${FetchContent_cmake_utils}_POPULATED)
  # Populate command can be used only once for every resource
  # during cmake configuration, which explains the conditional above.
  FetchContent_Populate(${FetchContent_cmake_utils})
endif()
message(STATUS "${FetchContent_cmake_utils}_SOURCE_DIR = ${${FetchContent_cmake_utils}_SOURCE_DIR}")
message(STATUS "${FetchContent_cmake_utils}_BINARY_DIR = ${${FetchContent_cmake_utils}_BINARY_DIR}")
include(${${FetchContent_cmake_utils}_SOURCE_DIR}/Utils.cmake)

set(FetchContent_conan_auto_install "conan_auto_install")
FetchContent_Declare(
  ${FetchContent_conan_auto_install}
  PREFIX external_dependencies/${FetchContent_conan_auto_install}
  GIT_REPOSITORY https://github.com/blockspacer/conan_auto_install.git
  GIT_TAG origin/master
  # Disable warning about detached HEAD https://stackoverflow.com/a/36794768
  GIT_CONFIG        advice.detachedHead=false
  SOURCE_DIR        "${CMAKE_BINARY_DIR}/${FetchContent_conan_auto_install}"
  BINARY_DIR        "${CMAKE_BINARY_DIR}/${FetchContent_conan_auto_install}-build"
  CMAKE_ARGS        "-Wno-dev"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
FetchContent_GetProperties(${FetchContent_conan_auto_install})
if (NOT ${FetchContent_conan_auto_install}_POPULATED)
  # Populate command can be used only once for every resource
  # during cmake configuration, which explains the conditional above.
  FetchContent_Populate(${FetchContent_conan_auto_install})
  # The second argument passed to add_subdirectory() is optional
  # and tells CMake, that we don’t want to build that directory in a
  # place basing on our repository structure (${CMAKE_CURRENT_BINARY_DIR})
  # but in a place specified by FetchContent.
  # This nicely separates out compilation products from the external ones.
  #add_subdirectory(
  #  ${${FetchContent_conan_auto_install}_SOURCE_DIR}
  #  ${${FetchContent_conan_auto_install}_BINARY_DIR})
endif()
message(STATUS "${FetchContent_conan_auto_install}_SOURCE_DIR = ${${FetchContent_conan_auto_install}_SOURCE_DIR}")
message(STATUS "${FetchContent_conan_auto_install}_BINARY_DIR = ${${FetchContent_conan_auto_install}_BINARY_DIR}")
include(${${FetchContent_conan_auto_install}_SOURCE_DIR}/conan-auto-install.cmake)
option(CONAN_AUTO_INSTALL "Let CMake call conan install automatically"
  OFF
)
if (CONAN_AUTO_INSTALL)
  set(CONAN_PROFILE
      "clang"
      CACHE STRING "Conan profile to use during installation")
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug" )
    set(conan_build_type "Release")
  else()
    set(conan_build_type "Debug")
  endif()
  conan_auto_install(
    CONAN_OPTIONS "--profile=${CONAN_PROFILE} -s build_type=${conan_build_type} --build=missing -o openssl:shared=True"
    #FORCE
  )
endif()

if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake")
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake OPTIONAL)
  conan_basic_setup(
    # prevent conan_basic_setup from resetting cmake variables
    TARGETS
    KEEP_RPATHS
    # see https://github.com/conan-io/conan/issues/6012
    NO_OUTPUT_DIRS
    )
else()
  message (FATAL_ERROR "must use conan")
endif()

find_package( EXPAT REQUIRED )
message(STATUS "EXPAT_LIBRARIES = ${EXPAT_LIBRARIES}")

find_package( ZLIB REQUIRED )
message(STATUS "ZLIB_LIBRARIES = ${ZLIB_LIBRARIES}")

find_package(cmake_platform_detection REQUIRED)
run_cmake_platform_detection()

find_package(cmake_build_options REQUIRED)
setup_default_build_type(RELEASE)
setup_cmake_build_options(RELEASE DEBUG)

find_package(cmake_helper_utils REQUIRED)

if(ENABLE_MSAN OR ENABLE_TSAN OR ENABLE_ASAN OR ENABLE_UBSAN)
  find_package(cmake_sanitizers REQUIRED)
endif()

#if(ENABLE_ASAN
#   OR ENABLE_TSAN
#   OR ENABLE_MSAN
#   OR ENABLE_UBSAN
#   #OR ENABLE_VALGRIND # TODO: valgrind support
#   )
#  list(APPEND BASIS_PUBLIC_DEFINES MEMORY_TOOL_REPLACES_ALLOCATOR=1)
#endif()

if(NOT TARGET CONAN_PKG::chromium_base)
  message(FATAL_ERROR "Use chromium_base from conan")
endif()

if(NOT TARGET CONAN_PKG::chromium_build_util)
  message(FATAL_ERROR "Use chromium_build_util from conan")
endif()

find_package(chromium_build_util REQUIRED)
#
if(TARGET chromium_build_util::chromium_build_util-static)
  set(build_util_LIB "chromium_build_util::chromium_build_util-static")
else()
  message(FATAL_ERROR "not supported: using system provided chromium_build_util library")
endif()

find_package(chromium_base REQUIRED)
if(TARGET chromium_base::chromium_base-static)
  set(base_LIB chromium_base::chromium_base-static)
else()
  message(FATAL_ERROR "not supported: using system provided chromium_base library")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/)

set(BASIS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/basis")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformChecks.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonOptions.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProjectFiles.cmake)

add_library(${STATIC_LIB_NAME}-test-includes INTERFACE)

# $<INSTALL_INTERFACE:...> is exported using install(EXPORT)
# $<BUILD_INTERFACE:...> is exported using export(), or when the target is used by another target in the same buildsystem
macro(add_relative_include_dir TARGET VISIBILITY_BUILD VISIBILITY_INSTALL NEW_ELEM)
  target_include_directories(${TARGET}
    ${VISIBILITY_BUILD} "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${NEW_ELEM}>"
    ${VISIBILITY_INSTALL} "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/${NEW_ELEM}>"
  )
  target_include_directories( ${STATIC_LIB_NAME}-test-includes SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/${NEW_ELEM} )
endmacro(add_relative_include_dir)

add_library(${STATIC_LIB_NAME} STATIC
  ${BASIS_SOURCES}
)

add_relative_include_dir(${STATIC_LIB_NAME} PRIVATE PRIVATE "basis")

add_relative_include_dir(${STATIC_LIB_NAME} PUBLIC PUBLIC "")

list(APPEND USED_3DPARTY_LIBS
  ${base_LIB}
  ${build_util_LIB}
)

# POSITION_INDEPENDENT_CODE for -fPIC
set_property(TARGET ${STATIC_LIB_NAME}
  PROPERTY POSITION_INDEPENDENT_CODE ON)

set_property(TARGET ${STATIC_LIB_NAME} PROPERTY CXX_STANDARD 17)

if(TARGET_EMSCRIPTEN)
  # use PROPERTY CXX_STANDARD 17
else()
  target_compile_features(${STATIC_LIB_NAME}
    PUBLIC cxx_auto_type
    PRIVATE cxx_variadic_templates)
endif()

target_compile_options(${STATIC_LIB_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall>)

target_compile_definitions(${STATIC_LIB_NAME} PRIVATE
  ${BASIS_PRIVATE_DEFINES}
)

# TODO: DISABLE_DOCTEST
list(APPEND BASIS_INTERFACE_DEFINES DISABLE_DOCTEST=1)

target_compile_definitions(${STATIC_LIB_NAME} PUBLIC
  ${BASIS_INTERFACE_DEFINES}
)

target_link_libraries(${STATIC_LIB_NAME} PRIVATE
  ${USED_3DPARTY_LIBS}
  #CONAN_PKG::chromium_base
  #CONAN_PKG::chromium_build_util
)

# TODO: DISABLE_DOCTEST
#target_link_libraries(${STATIC_LIB_NAME} PUBLIC
#  CONAN_PKG::doctest
#)

set(DEBUG_LIBRARY_SUFFIX "")
set_target_properties(${STATIC_LIB_NAME}
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" # TODO: /lib
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" # TODO: /lib
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" # TODO: /bin
    OUTPUT_NAME "${STATIC_LIB_NAME}$<$<CONFIG:Debug>:${DEBUG_LIBRARY_SUFFIX}>"
)

## ---------------------------- valgrind -------------------------------- ##

if(ENABLE_VALGRIND)
  check_valgrind_config()
endif()

## ---------------------------- sanitizers -------------------------------- ##

if(ENABLE_MSAN OR ENABLE_TSAN OR ENABLE_ASAN OR ENABLE_UBSAN)
  # use llvm_tools from conan
  find_program_helper(llvm-symbolizer
    PATHS
      #${CONAN_BIN_DIRS}
      ${CONAN_BIN_DIRS_LLVM_TOOLS}
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    ${ARGUMENTS_UNPARSED_ARGUMENTS}
    REQUIRED
    OUT_VAR LLVM_SYMBOLIZER_PROGRAM
    VERBOSE TRUE
  )

  check_sanitizer_options(
    ENABLE_TSAN ${ENABLE_TSAN}
    ENABLE_ASAN ${ENABLE_ASAN}
    ENABLE_MSAN ${ENABLE_MSAN}
    ENABLE_UBSAN ${ENABLE_UBSAN}
    LLVM_SYMBOLIZER_PROGRAM ${LLVM_SYMBOLIZER_PROGRAM}
  )
endif()

if(ENABLE_MSAN)
  message(STATUS "enabling MSAN on ${STATIC_LIB_NAME}")
  add_msan_static_link(${STATIC_LIB_NAME})
  add_msan_flags(${STATIC_LIB_NAME})
endif(ENABLE_MSAN)

if(ENABLE_TSAN)
  message(STATUS "enabling TSAN on ${STATIC_LIB_NAME}")
  add_tsan_static_link(${STATIC_LIB_NAME})
  add_tsan_flags(${STATIC_LIB_NAME})
endif(ENABLE_TSAN)

if(ENABLE_ASAN)
  message(STATUS "enabling ASAN on ${STATIC_LIB_NAME}")
  add_asan_static_link(${STATIC_LIB_NAME})
  add_asan_flags(${STATIC_LIB_NAME})
endif(ENABLE_ASAN)

if(ENABLE_UBSAN)
  message(STATUS "enabling UBSAN on ${STATIC_LIB_NAME}")
  add_ubsan_static_link(${STATIC_LIB_NAME})
  add_ubsan_flags(${STATIC_LIB_NAME})
endif(ENABLE_UBSAN)

# install and export steps
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Exports.cmake)

if(ENABLE_TESTS)
  add_subdirectory( tests )
endif()
