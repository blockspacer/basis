cmake_minimum_required( VERSION 3.13.3 FATAL_ERROR )

set(ROOT_PROJECT_NAME ${PROJECT_NAME})
set(ROOT_PROJECT_LIB ${STATIC_LIB_NAME})
set(${ROOT_PROJECT_NAME}_CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

set( PROJECT_NAME "${ROOT_PROJECT_NAME}-tests" )
set( PROJECT_DESCRIPTION "unit tests" )
set( ${PROJECT_NAME}_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${PROJECT_NAME}_PROJECT_DIR" )

# Get CMAKE_MODULE_PATH from parent project
#include( ${ROOT_PROJECT_DIR}/cmake/Utils.cmake )
#set_cmake_module_paths( ${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR};${${ROOT_PROJECT_NAME}_CMAKE_MODULE_PATH}" ) # from Utils.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${${ROOT_PROJECT_NAME}_CMAKE_MODULE_PATH}")

if (ENABLE_CODE_COVERAGE)
  set( CATCH2_TEST_ARGS -r junit -o unit_test_result.xml )
else()
  set( CATCH2_TEST_ARGS -r console --use-colour yes --order rand --durations yes --warn NoTests --invisibles)
endif()

set( GTEST_TEST_ARGS --gtest_shuffle)

# Run ALL tests with verbose output
# Usage: cmake --build build --target run_all_tests
add_custom_target(${ROOT_PROJECT_NAME}_run_all_tests
    DEPENDS ${ROOT_PROJECT_LIB}
)

add_library(${ROOT_PROJECT_NAME}-catch2-root INTERFACE)
target_include_directories( ${ROOT_PROJECT_NAME}-catch2-root SYSTEM INTERFACE
  ${CONAN_CATCH2_ROOT}/include/catch2 )
get_target_property (FakeIt_IMPORTED_LOCATION CONAN_PKG::catch2 INTERFACE_INCLUDE_DIRECTORIES)
message( "CONAN_PKG::catch2=${catch2_IMPORTED_LOCATION}" )
get_target_property (FakeIt_IMPORTED_LOCATION CONAN_PKG::FakeIt INTERFACE_INCLUDE_DIRECTORIES)
message( "CONAN_PKG::FakeIt=${FakeIt_IMPORTED_LOCATION}" )

list(APPEND TESTS_3DPARTY_LIBS
    #FakeIt
    CONAN_PKG::conan_gtest
    CONAN_PKG::FakeIt
    CONAN_PKG::catch2
    ${ROOT_PROJECT_NAME}-catch2-root # so FakeIt will be able to see 'catch.hpp'
    ${USED_3DPARTY_LIBS}
    ${ROOT_PROJECT_LIB}-test-includes
)

include( testRunner ) # start tests as CMake targets

#findPackageCrossPlatform( Sanitizers ) # see https://github.com/arsenm/sanitizers-cmake
# sanitizer_add_blacklist_file("blacklist.txt") # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support

macro(add_to_tests_list source_list)
  list(APPEND UNIT_TEST_SOURCE_LIST ${source_list})
endmacro()

macro(set_test_compile_options target)
  target_compile_options(${target} PRIVATE
              $<$<CXX_COMPILER_ID:MSVC>:
              /W3 # Set warning level
              #/WX # Treats all compiler warnings as errors.
              >
              $<$<CXX_COMPILER_ID:GNU>:
              -Wformat=2
              -Wall
              -W
##              "$<$<CONFIG:RELEASE>:-Werror>" # Treats all compiler warnings as errors.
              -Wpedantic
              #-fsanitize-address-use-after-scope # https://clang.llvm.org/docs/AddressSanitizer.html
              #-fsanitize=pointer-compare # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
              -fno-omit-frame-pointer # https://github.com/google/sanitizers/wiki/AddressSanitizer#using-addresssanitizer
              >
              $<$<CXX_COMPILER_ID:Clang>:
              -Wformat=2
              -Wall
              -W
##              "$<$<CONFIG:RELEASE>:-Werror>" # Treats all compiler warnings as errors.
              -Wpedantic
              -Wdeprecated-register
              #-fsanitize-address-use-after-scope # https://clang.llvm.org/docs/AddressSanitizer.html
              #-fsanitize=pointer-compare # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
              -fno-omit-frame-pointer # https://github.com/google/sanitizers/wiki/AddressSanitizer#using-addresssanitizer
              >
          )

  # @see https://github.com/eranpeer/FakeIt
  target_compile_options( ${target} PRIVATE
    # If you don't use /ZI, you will have exceptions mocking destructors (which includes unique_ptr and other smart pointers).
    $<$<CXX_COMPILER_ID:MSVC>:/ZI>
    # On GCC, optimization flag O2 and O3 are not supported. You must compile the test project with -O1 or -O0.
    $<$<CXX_COMPILER_ID:GNU>:-O0>
    $<$<CXX_COMPILER_ID:Clang>:-O0> )

  set_target_properties( ${target} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CMAKE_CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${ROOT_PROJECT_NAME}-tests )
endmacro()

macro(tests_add_executable target source_list TEST_ARGS TEST_LIB)
  list(APPEND UNIT_TEST_TARGETS ${target})

  add_to_tests_list(${source_list})

  add_executable(${target} ${source_list})

  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug" )
##    add_sanitizers( ${target} ) # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support
  endif()

  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(${target} PRIVATE
    # 3dparty libs
    ${TESTS_3DPARTY_LIBS}
    # system libs
    ${USED_SYSTEM_LIBS}
    # main project lib
    ${ROOT_PROJECT_LIB}
    ${TEST_LIB}
  )

  set_test_compile_options( ${target} )

  add_test(
    NAME ${target}
    COMMAND ${target} ${TEST_ARGS})

  if(ENABLE_DOCTEST)
    target_link_libraries(${target} PUBLIC
      CONAN_PKG::doctest
    )

    target_compile_definitions( ${target} PUBLIC
      USE_DOCTEST_TEST=1
      BASIS_RUN_DOCTEST_TESTS=1 )
  endif()

  add_test_command_targets( ${target} )

  if(ENABLE_VALGRIND)
    # |base::TestSuite| tries to kill test process
    # but valgrind wraps process, so nothing to kill
    target_compile_definitions( ${target} PUBLIC
      GTEST_NO_SUITE=1) # TODO: valgrind may not support chromium base, FIXME. And remove GTEST_NO_SUITE
  endif(ENABLE_VALGRIND)

  # USAGE:
  # cmake -E time cmake --build . --target TARGET_NAME_run_valgrind
  valgrind_enabler(
    IS_ENABLED
      ${ENABLE_VALGRIND}
    STANDALONE_TARGET
      ${target}
    EXTRA_OPTIONS # see https://valgrind.org/docs/manual/manual-core.html
      # When enabled, Valgrind will trace into sub-processes
      # initiated via the exec system call.
      # This is necessary for multi-process programs.
      --trace-children=yes
      # --leak-check=full:
      # each individual leak will be shown in detail
      --leak-check=full
      # --show-leak-kinds=all:
      # Show all of "definite, indirect, possible, reachable"
      # leak kinds in the "full" report.
      --show-leak-kinds=all
      # --track-origins=yes:
      # Favor useful output over speed.
      # This tracks the origins of uninitialized values,
      # which could be very useful for memory errors.
      # Consider turning off if Valgrind is unacceptably slow.
      --track-origins=yes
      # --verbose:
      # Can tell you about unusual behavior of your program.
      # Repeat for more verbosity.
      --verbose
      --show-reachable=yes
      # When enabled, Valgrind stops reporting errors
      # after 10,000,000 in total, or 1,000 different ones,
      # have been seen.
      --error-limit=yes
      # is option is particularly useful with C++ programs,
      # as it prints out the suppressions with mangled names,
      # as required.
      --gen-suppressions=all
      # Write to a file.
      # Useful when output exceeds terminal space.
      --log-file=${CMAKE_CURRENT_BINARY_DIR}/${ROOT_PROJECT_NAME}-tests/${target}_valgrind_raw.log
      # executable
      $<TARGET_FILE:${target}>
      # arguments of executable
      ${TEST_ARGS}
    VERBOSE
    REQUIRED
  )
  if(ENABLE_VALGRIND AND ENABLE_VALGRIND_TESTS)
    add_dependencies(${ROOT_PROJECT_NAME}_run_all_tests
      ${target}_run_valgrind
    )
  endif(ENABLE_VALGRIND AND ENABLE_VALGRIND_TESTS)

  if(AUTORUN_TESTS)
    add_test_autostarter( ${target} )
  endif()
endmacro()

# Use with files that contain
# `int main(...) { ... }`
macro(define_doctest_implementation_in_file source_list)
  # DOCTEST_CONFIG_IMPLEMENT:
  # If the client wants to supply the main() function
  # (either to set an option with some value from the code
  # or to integrate the framework into his existing project codebase)
  # this identifier should be used.
  # This should be defined only in the source file
  # where the library is implemented.
  set_property(
    SOURCE ${source_list}
    APPEND
    PROPERTY COMPILE_DEFINITIONS
    DOCTEST_CONFIG_IMPLEMENT=1
  )
endmacro()

set(test_main_catch "${ROOT_PROJECT_NAME}-test_main_catch")
add_library( ${test_main_catch} OBJECT
  main_catch.cpp
  )
define_doctest_implementation_in_file("main_catch.cpp")

set_property(TARGET ${test_main_catch} PROPERTY CXX_STANDARD 17)

# ensure that dependencies build before <target> does.
add_dependencies(${test_main_catch} ${ROOT_PROJECT_LIB})

set_test_compile_options( ${test_main_catch} )

target_link_libraries(${test_main_catch} PUBLIC
  # 3dparty libs
  ${TESTS_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
  # main project lib
  ${ROOT_PROJECT_LIB}
)

target_compile_definitions( ${test_main_catch} PUBLIC
  "-DUSE_CATCH_TEST=1" )

if(ENABLE_VALGRIND)
  # |base::TestSuite| tries to kill test process
  # but valgrind wraps process, so nothing to kill
  target_compile_definitions( ${test_main_catch} PUBLIC
    GTEST_NO_SUITE=1) # TODO: valgrind may not support chromium base, FIXME. And remove GTEST_NO_SUITE
endif(ENABLE_VALGRIND)

set(test_main_gtest "${ROOT_PROJECT_NAME}-test_main_gtest")
add_library( ${test_main_gtest} OBJECT
  main_gtest.cpp
  )
define_doctest_implementation_in_file("main_gtest.cpp")

set_property(TARGET ${test_main_gtest} PROPERTY CXX_STANDARD 17)

# ensure that dependencies build before <target> does.
add_dependencies(${test_main_gtest} ${ROOT_PROJECT_LIB})

set_test_compile_options( ${test_main_gtest} )

target_link_libraries(${test_main_gtest} PUBLIC
  # 3dparty libs
  ${TESTS_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
  # main project lib
  ${ROOT_PROJECT_LIB}
)

target_compile_definitions( ${test_main_gtest} PUBLIC
  "-DUSE_GTEST_TEST=1" )

if(ENABLE_VALGRIND)
  # |base::TestSuite| tries to kill test process
  # but valgrind wraps process, so nothing to kill
  target_compile_definitions( ${test_main_gtest} PUBLIC
    GTEST_NO_SUITE=1) # TODO: valgrind may not support chromium base, FIXME. And remove GTEST_NO_SUITE
endif(ENABLE_VALGRIND)

if(ENABLE_DOCTEST)
  # see https://github.com/onqtam/doctest/blob/master/doc/markdown/commandline.md
  set( DOCTEST_TEST_ARGS --duration=1)

  set(test_main_doctest "${ROOT_PROJECT_NAME}-test_main_doctest")
  add_library( ${test_main_doctest} OBJECT
    tests_common.h # include in IDE
  )

  set_property(TARGET ${test_main_doctest} PROPERTY CXX_STANDARD 17)

  # ensure that dependencies build before <target> does.
  add_dependencies(${test_main_doctest} ${ROOT_PROJECT_LIB})

  set_test_compile_options( ${test_main_doctest} )

  target_link_libraries(${test_main_doctest} PUBLIC
    CONAN_PKG::doctest
    # 3dparty libs
    ${TESTS_3DPARTY_LIBS}
    # system libs
    ${USED_SYSTEM_LIBS}
    # main project lib
    ${ROOT_PROJECT_LIB}
  )

  target_compile_definitions( ${test_main_doctest} PUBLIC
    USE_DOCTEST_TEST=1
    BASIS_RUN_DOCTEST_TESTS=1 )

  target_link_libraries(${test_main_gtest} PUBLIC
    CONAN_PKG::doctest
  )

  target_compile_definitions( ${test_main_gtest} PUBLIC
    USE_DOCTEST_TEST=1
    BASIS_RUN_DOCTEST_TESTS=1 )

  target_link_libraries(${test_main_catch} PUBLIC
    CONAN_PKG::doctest
  )

  target_compile_definitions( ${test_main_catch} PUBLIC
    USE_DOCTEST_TEST=1
    BASIS_RUN_DOCTEST_TESTS=1 )
endif()

#target_link_libraries( test_main PUBLIC
#  sol2::sol2_single Catch FakeIt boost_beast nlohmann_json microsoft_gsl boost_outcome better_enums )

#target_compile_definitions( test_main PUBLIC
#  ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

#target_link_directories( test_main PUBLIC ${WEBRTC_LIB_PATHS} )

#target_include_directories( test_main SYSTEM PUBLIC
#  ${ROOT_PROJECT_DIR}/src
#  ${WEBRTC_INCLUDES}
#  ${THIRDPARTY_FILES}
#  ${ABSEIL_BASE_IMPORTED_LOCATION})

option(AUTORUN_TESTS "Autorun tests" OFF) # see add_test_autostarter

option(${ROOT_PROJECT_NAME}_BUILD_TESTS "Enable tests" ON)
if(${ROOT_PROJECT_NAME}_BUILD_TESTS)
  message( "${PROJECT_NAME} testing enabled" )

  set ( pimpl_deps
    pimpl.test.cpp
    pimpl.impl.cpp
  )
  tests_add_executable(${ROOT_PROJECT_NAME}-pimpl
    "${pimpl_deps}" "${CATCH2_TEST_ARGS}" "${test_main_catch}")

  tests_add_executable(${ROOT_PROJECT_NAME}-gmock
    "gmock.test.cpp" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-i18n
    "i18n.test.cpp" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-money
    "${BASIS_DIR}/money/money.test.cpp" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-usage_limiter
    "${BASIS_DIR}/statistics/usage_limiter_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-expandable_statistics_window
    "${BASIS_DIR}/statistics/expandable_statistics_window_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-moving_average
    "${BASIS_DIR}/statistics/moving_average_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-moving_median_filter
    "${BASIS_DIR}/statistics/moving_median_filter_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-moving_statistics_window
    "${BASIS_DIR}/statistics/moving_statistics_window_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-percentile_filter
    "${BASIS_DIR}/statistics/percentile_filter_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-quality_threshold
    "${BASIS_DIR}/statistics/quality_threshold_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-rate_tracker
    "${BASIS_DIR}/statistics/rate_tracker_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-top_n
    "${BASIS_DIR}/algorithm/top_n_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-bind_to_task_runner
    "${BASIS_DIR}/bind/bind_to_task_runner_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-serializers
    "${BASIS_DIR}/serialization/serializers_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-static_sequence
    "${BASIS_DIR}/static_sequence/static_sequence_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-status
    "${BASIS_DIR}/status/status_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-statusor
    "${BASIS_DIR}/status/statusor_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-with_details
    "${BASIS_DIR}/status/with_details_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-thread_health_checker
    "${BASIS_DIR}/threading/thread_health_checker_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-system_time_change_notifier
    "${BASIS_DIR}/time/system_time_change_notifier_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-backoff_timer
    "${BASIS_DIR}/time/backoff_timer.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-observable
    "${BASIS_DIR}/core/observable_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-weak_handle
    "${BASIS_DIR}/core/weak_handle_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-spacer
    "${BASIS_DIR}/log/spacer_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-table_printer
    "${BASIS_DIR}/log/table_printer_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-unique_ptr_wrap
    "${BASIS_DIR}/unique_ptr_wrap_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-file_path_util
    "${BASIS_DIR}/files/file_path_util_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-dependency_hierarchy
    "${BASIS_DIR}/dependency_hierarchy/dependency_hierarchy_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-dependency_util
    "${BASIS_DIR}/dependency_hierarchy/dependency_util_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  if(ENABLE_DOCTEST)
    tests_add_executable(${ROOT_PROJECT_NAME}-doctest_test
      "doctest_test.cc" "${DOCTEST_TEST_ARGS}" "${test_main_doctest}")
    define_doctest_implementation_in_file("doctest_test.cc")
  endif()

  # FIXME: run only under ASAN
  #tests_add_executable(${ROOT_PROJECT_NAME}-unowned_ptr
  #  "${BASIS_DIR}/unowned_ptr_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  # FIXME: use base::WrappedPromise
  # Check failed: !promise_. The PassedPromise must be Cleared or passed onto a Wrapped Promise
  #tests_add_executable(${ROOT_PROJECT_NAME}-abstract_promise
  #  "${BASIS_DIR}/promise/abstract_promise_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-dependent_list
    "${BASIS_DIR}/promise/dependent_list_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  # FIXME: use base::WrappedPromise
  # Check failed: !promise_. The PassedPromise must be Cleared or passed onto a Wrapped Promise
  #tests_add_executable(${ROOT_PROJECT_NAME}-helpers
  #  "${BASIS_DIR}/promise/helpers_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-post_task_executor
    "${BASIS_DIR}/promise/post_task_executor_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-promise
    "${BASIS_DIR}/promise/promise_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-promise_value
    "${BASIS_DIR}/promise/promise_value_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-backoff_entry_serializer
    "${BASIS_DIR}/backoff_entry/backoff_entry_serializer_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-backoff_entry
    "${BASIS_DIR}/backoff_entry/backoff_entry_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-interval
    "${BASIS_DIR}/interval/interval_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-interval_set
    "${BASIS_DIR}/interval/interval_set_test.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  set ( fakeit_deps
    fakeit.test.cpp
  )
  tests_add_executable(${ROOT_PROJECT_NAME}-fakeit
    "${fakeit_deps}" "${CATCH2_TEST_ARGS}" "${test_main_catch}")

  tests_add_executable(${ROOT_PROJECT_NAME}-alarm_manager
    "${BASIS_DIR}/task/alarm_manager_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-prioritized_once_task_heap
    "${BASIS_DIR}/task/prioritized_once_task_heap_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-prioritized_list
    "${BASIS_DIR}/container/prioritized_list_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-prioritized_job_dispatcher
    "${BASIS_DIR}/container/prioritized_job_dispatcher_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-prioritized_repeating_task_list
    "${BASIS_DIR}/container/prioritized_repeating_task_list_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  tests_add_executable(${ROOT_PROJECT_NAME}-uint128
    "${BASIS_DIR}/numerics/uint128_unittest.cc" "${GTEST_TEST_ARGS}" "${test_main_gtest}")

  #add_to_tests_list(utils)
  #add_to_tests_list(pimpl)

  #tests_add_executable(check_all ${UNIT_TEST_SOURCE_LIST} ${GTEST_TEST_ARGS})
  #message(FATAL_ERROR ${UNIT_TEST_SOURCE_LIST})

  # "pimpl" is one of test program names
  add_custom_command( TARGET ${ROOT_PROJECT_NAME}-pimpl POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_directory
                          ${CMAKE_CURRENT_SOURCE_DIR}/data
                          ${CMAKE_CURRENT_BINARY_DIR}/${ROOT_PROJECT_NAME}-tests )

  enable_testing()
endif()

foreach(target IN LISTS ALL_TESTS_LIST_VERBOSE)
  add_dependencies(${ROOT_PROJECT_NAME}_run_all_tests
    ${target}
  )
endforeach()

option(ENABLE_CODE_COVERAGE "ENABLE CODE COVERAGE" OFF)
if (ENABLE_CODE_COVERAGE)
  set(LCOV_REMOVE_EXTRA "'*/src/include/webrtc/*'" "'${ROOT_PROJECT_DIR}/lib/*'" "'${ROOT_PROJECT_DIR}/submodules/*'" "'test/*'" "'ext/*'" "'abseil/*'" "'submodules/*'" "'*/tests/*'" "'scripts/*'" "'lib/*'" "'build/*'" "'bin/*'" "'*Catch/*'" "'*FakeIt/*'")
  set(CMAKE_BUILD_TYPE Coverage)
  # test coverage build configuration for C++: debug build, no optimization, profiling enabled
  #set(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")

  target_compile_options(${ROOT_PROJECT_NAME}_run_all_tests PRIVATE
    -g -O0 -Wall -fprofile-arcs -ftest-coverage
  )
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  # include the cmake code coverage module
  include( CodeCoverage )
  # cmake code coverage module target
  setup_target_for_coverage(
    # Name for custom target.
    unit_tests_coverage
    # Name of the test driver executable that runs the tests.
    # NOTE! This should always have a ZERO as exit code
    # otherwise the coverage generation will not complete.
    check_all
    coverage # Name of output directory.
    )
  message("added make code coverage for ${PROJECT_NAME}_coverage")
else()
  message(WARNING "code coverage off")
endif()

#find_package(Sanitizers)
#if(HAS_Address_SANITIZER AND HAS_Fuzzer_SANITIZER)
#	add_executable(fuzz fuzzing.cpp)
#	target_link_libraries(fuzz PUBLIC gltfpp Sanitizer::Fuzzer Sanitizer::Address)
#endif()
