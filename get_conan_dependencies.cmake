if(EXISTS "${CURRENT_SCRIPT_DIR}/.tmp")
  cmake_remove_directory("${CURRENT_SCRIPT_DIR}/.tmp")
endif()

cmake_make_dir("${CURRENT_SCRIPT_DIR}/.tmp")


# chromium_base_conan

if(NOT EXISTS "${CURRENT_SCRIPT_DIR}/.tmp/chromium_base_conan")
  git_clone("${CURRENT_SCRIPT_DIR}/.tmp/chromium_base_conan"
      "http://github.com/blockspacer/chromium_base_conan.git"
      "")
endif()
conan_build_target_if(
  "chromium_base" # target to clean
  "conan/stable"
  "${CURRENT_SCRIPT_DIR}/.tmp/chromium_base_conan" # target to build
  ALWAYS_BUILD
  "")

# corrade

if(NOT EXISTS "${CURRENT_SCRIPT_DIR}/.tmp/corrade")
  git_clone("${CURRENT_SCRIPT_DIR}/.tmp/corrade"
      "https://github.com/blockspacer/corrade_conan.git"
      "")
endif()
conan_build_target_if(
  "corrade" # target to clean
  "conan/stable"
  "${CURRENT_SCRIPT_DIR}/.tmp/corrade" # target to build
  ALWAYS_BUILD
  "")

# type_safe

if(NOT EXISTS "${CURRENT_SCRIPT_DIR}/.tmp/type_safe")
  git_clone("${CURRENT_SCRIPT_DIR}/.tmp/type_safe"
      "http://github.com/foonathan/type_safe.git"
      "-b;v0.2.1")
endif()
conan_build_target_if(
  "type_safe" # target to clean
  "conan/stable"
  "${CURRENT_SCRIPT_DIR}/.tmp/type_safe" # target to build
  ALWAYS_BUILD
  "")
