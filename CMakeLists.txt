# see https://github.com/chromium/chromium/blob/9db0b5162b8648833c3a6b1af044cebd9d7d3855/base/BUILD.gn

cmake_minimum_required(VERSION 3.4)

# project() must be before checks, see https://stackoverflow.com/a/26437667/10904212
set(PROJECT_NAME "basis")
set(PROJECT_VERSION 1.0.0.1)
project(${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  LANGUAGES CXX C)

set(STATIC_LIB_NAME ${PROJECT_NAME})

# ------------------------------- Options --------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Options.cmake)

# --------------------------- conan configuration ------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/fetch_cmake_utils.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/fetch_conan_auto_install.cmake)

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
  # You can use `EXTRA_CONAN_AUTO_INSTALL_OPTIONS` like so:
  # cmake .. -DCONAN_AUTO_INSTALL=ON -DEXTRA_CONAN_AUTO_INSTALL_OPTIONS="--build missing" -DCMAKE_BUILD_TYPE=Debug
  conan_auto_install(
    CONAN_OPTIONS "--profile=${CONAN_PROFILE} -s build_type=${conan_build_type} -s llvm_tools:build_type=Release --build=missing -o openssl:shared=True ${EXTRA_CONAN_AUTO_INSTALL_OPTIONS}"
    #FORCE
  )
endif()

if(COMPILE_WITH_LLVM_TOOLS)
  message(STATUS
    "Using COMPILE_WITH_LLVM_TOOLS")
  # do not check compile in conanbuildinfo
  # cause we will switch to other compiler after conan install
  set(CONAN_DISABLE_CHECK_COMPILER ON)
else(COMPILE_WITH_LLVM_TOOLS)
  message(STATUS
    "Disabled COMPILE_WITH_LLVM_TOOLS")
endif(COMPILE_WITH_LLVM_TOOLS)

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
  message (FATAL_ERROR "must use conan: ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake")
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

if(ENABLE_MSAN)
  add_msan_flags()
endif(ENABLE_MSAN)

if(ENABLE_TSAN)
  add_tsan_flags()
endif(ENABLE_TSAN)

if(ENABLE_ASAN)
  add_asan_flags()
endif(ENABLE_ASAN)

if(ENABLE_UBSAN)
  add_ubsan_flags()
endif(ENABLE_UBSAN)

if(COMPILE_WITH_LLVM_TOOLS)
  message(STATUS
    "Using COMPILE_WITH_LLVM_TOOLS")
  # force change CMAKE_*_COMPILER and CMAKE_LINKER to clang from conan
  compile_with_llvm_tools() # from cmake_helper_utils (conan package)
else(COMPILE_WITH_LLVM_TOOLS)
  message(STATUS
    "Disabled COMPILE_WITH_LLVM_TOOLS")
endif(COMPILE_WITH_LLVM_TOOLS)

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
  message(FATAL_ERROR "not supported ${build_util_LIB}: using system provided chromium_build_util library")
endif()

find_package(chromium_base REQUIRED)
if(NOT TARGET ${base_LIB})
  message(FATAL_ERROR "not supported ${base_LIB}: using system provided chromium_base library")
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

target_compile_definitions(${STATIC_LIB_NAME} PUBLIC
  ${BASIS_PRIVATE_DEFINES}
  # https://stackoverflow.com/a/30877725
  BOOST_SYSTEM_NO_DEPRECATED
  BOOST_ERROR_CODE_HEADER_ONLY
  # TODO: make program_options work without rtti (boost::bad_any_cast)
  #BOOST_NO_RTTI
  #BOOST_NO_TYPEID
  # TODO
  #BOOST_NO_EXCEPTIONS
  BOOST_ASIO_STANDALONE=1
)

target_link_libraries(${STATIC_LIB_NAME} PUBLIC
  ${USED_3DPARTY_LIBS}
)

if(ENABLE_DOCTEST)
  message(STATUS "DOCTEST Enabled")
  target_link_libraries(${STATIC_LIB_NAME} PUBLIC
    CONAN_PKG::doctest
  )
else()
  message(STATUS "DOCTEST Disabled")
  target_compile_definitions(${STATIC_LIB_NAME} PUBLIC
    DISABLE_DOCTEST=1
  )
endif()

# see https://doc.magnum.graphics/corrade/corrade-cmake.html#corrade-cmake-subproject
find_package(Corrade REQUIRED PluginManager)

target_link_libraries(${STATIC_LIB_NAME} PRIVATE
  CONAN_PKG::openssl
)

target_link_libraries(${STATIC_LIB_NAME} PUBLIC
  CONAN_PKG::boost
  CONAN_PKG::entt
  Corrade::PluginManager
)

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

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/setup_sanitizers.cmake)

# install and export steps
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Exports.cmake)

if(ENABLE_TESTS)
  add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/tests )
endif()
