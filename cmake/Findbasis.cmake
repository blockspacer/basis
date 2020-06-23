get_filename_component(CURRENT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
list(APPEND CMAKE_MODULE_PATH ${CURRENT_CMAKE_DIR})

#include(CMakeFindDependencyMacro) # use find_package instead

# NOTE: some packages may be optional (platform-specific, etc.)
# find_package(... REQUIRED)
find_package(chromium_base REQUIRED)
find_package(boost QUIET)
# see https://doc.magnum.graphics/corrade/corrade-cmake.html#corrade-cmake-subproject
find_package(Corrade REQUIRED PluginManager)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET CONAN_PKG::boost)
  message(FATAL_ERROR "Use boost from conan")
endif()

if(NOT TARGET CONAN_PKG::basis)
  message(FATAL_ERROR "Use basis from conan")
endif()
set(basis_LIB CONAN_PKG::basis)
# conan package has '/include' dir
set(basis_HEADER_DIR
  ${CONAN_BASIS_ROOT}/include
)
if(basis_LOCAL_BUILD)
  # name of created target
  set(basis_LIB basis)
  # no '/include' dir on local build
  set(basis_HEADER_DIR
    ${CONAN_BASIS_ROOT}
  )
else()
  # uses Config.cmake or a -config.cmake file
  # see https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/How-to-create-a-ProjectConfig.cmake-file
  # BELOW MUST BE EQUAL TO find_package(... CONFIG REQUIRED)
  # NOTE: find_package(CONFIG) not supported with EMSCRIPTEN, so use include()
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/basis-config.cmake)
endif(basis_LOCAL_BUILD)
message(STATUS "basis_HEADER_DIR=${basis_HEADER_DIR}")
