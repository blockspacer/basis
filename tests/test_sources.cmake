basis_test_gtest(${ROOT_PROJECT_NAME}-gmock "gmock.test.cpp")

basis_test_gtest(${ROOT_PROJECT_NAME}-i18n "i18n.test.cpp")

# "i18n" is one of test program names
add_custom_command( TARGET ${ROOT_PROJECT_NAME}-i18n POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_CURRENT_SOURCE_DIR}/data
                        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} )

list(APPEND basis_unittests
  annotations/asio_guard_annotations_unittest.cc
  threading/thread_health_checker_unittest.cc
  task/prioritized_once_task_heap_unittest.cc
  task/alarm_manager_unittest.cc
  ECS/ecs_hierarchies_unittest.cc
)
list(APPEND basis_unittest_utils
  #"allocator/partition_allocator/arm_bti_test_functions.h"
)

list(REMOVE_DUPLICATES basis_unittests)
list(TRANSFORM basis_unittests PREPEND ${BASIS_SOURCES_PATH})

list(REMOVE_DUPLICATES basis_unittest_utils)
list(FILTER basis_unittest_utils EXCLUDE REGEX ".*_unittest.cc$")
list(TRANSFORM basis_unittest_utils PREPEND ${BASIS_SOURCES_PATH})

foreach(FILEPATH ${basis_unittests})
  set(test_sources
    "${FILEPATH}"
    ${basis_unittest_utils}
  )
  list(REMOVE_DUPLICATES basis_unittest_utils)
  get_filename_component(FILENAME_WITHOUT_EXT ${FILEPATH} NAME_WE)
  basis_test_gtest(${ROOT_PROJECT_NAME}-basis-${FILENAME_WITHOUT_EXT}
    "${test_sources}")
endforeach()
