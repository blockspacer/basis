from conans import ConanFile, CMake, tools, AutoToolsBuildEnvironment, RunEnvironment, python_requires
from conans.errors import ConanInvalidConfiguration, ConanException
from conans.tools import os_info
import os, re, stat, fnmatch, platform, glob, traceback, shutil
from functools import total_ordering

# if you using python less than 3 use from distutils import strtobool
from distutils.util import strtobool

# conan runs the methods in this order:
# config_options(),
# configure(),
# requirements(),
# package_id(),
# build_requirements(),
# build_id(),
# system_requirements(),
# source(),
# imports(),
# build(),
# package(),
# package_info()

conan_build_helper = python_requires("conan_build_helper/[~=0.0]@conan/stable")

# Users locally they get the 1.0.0 version,
# without defining any env-var at all,
# and CI servers will append the build number.
# USAGE
# version = get_version("1.0.0")
# BUILD_NUMBER=-pre1+build2 conan export-pkg . my_channel/release
def get_version(version):
    bn = os.getenv("BUILD_NUMBER")
    return (version + bn) if bn else version

class basis_conan_project(conan_build_helper.CMakePackage):
    name = "basis"

    # Indicates License type of the packaged library
    # TODO (!!!)
    # license = "MIT"

    version = get_version("master")

    # TODO (!!!)
    #url = "https://github.com/blockspacer/CXXCTP"

    description = "`basis` library"
    topics = ('c++')

    options = {
        "shared": [True, False],
        "debug": [True, False],
        "enable_ubsan": [True, False],
        "enable_asan": [True, False],
        "enable_msan": [True, False],
        "enable_tsan": [True, False],
        "enable_valgrind": [True, False]
    }

    default_options = (
        "shared=False",
        "debug=False",
        "enable_ubsan=False",
        "enable_asan=False",
        "enable_msan=False",
        "enable_tsan=False",
        "enable_valgrind=False",
        "*:integration=catch", # for FakeIt,
        # chromium_base
        "chromium_base:use_alloc_shim=True",
        # chromium_tcmalloc
        "chromium_tcmalloc:use_alloc_shim=True",
        # openssl
        "openssl:shared=True",
    )

    # Custom attributes for Bincrafters recipe conventions
    _source_subfolder = "."
    _build_subfolder = "."

    # NOTE: no cmake_find_package due to custom FindXXX.cmake
    generators = "cmake", "cmake_paths", "virtualenv"

    # Packages the license for the conanfile.py
    #exports = ["LICENSE.md"]

    # If the source code is going to be in the same repo as the Conan recipe,
    # there is no need to define a `source` method. The source folder can be
    # defined like this
    exports_sources = ("LICENSE", "VERSION", "*.md", "include/*", "src/*",
                       "cmake/*", "examples/*", "CMakeLists.txt", "tests/*", "benchmarks/*",
                       "scripts/*", "tools/*", "codegen/*", "assets/*",
                       "docs/*", "licenses/*", "patches/*", "resources/*",
                       "submodules/*", "thirdparty/*", "third-party/*",
                       "third_party/*", "basis/*")

    settings = "os", "compiler", "build_type", "arch"

    # sets cmake variables required to use clang 10 from conan
    def _is_compile_with_llvm_tools_enabled(self):
      return self._environ_option("COMPILE_WITH_LLVM_TOOLS", default = 'false')

    # installs clang 10 from conan
    def _is_llvm_tools_enabled(self):
      return self._environ_option("ENABLE_LLVM_TOOLS", default = 'false')

    def _is_cppcheck_enabled(self):
      return self._environ_option("ENABLE_CPPCHECK", default = 'false')

    #def source(self):
    #  url = "https://github.com/....."
    #  self.run("git clone %s ......." % url)

    def configure(self):
        lower_build_type = str(self.settings.build_type).lower()

        if lower_build_type != "release" and not self._is_llvm_tools_enabled():
            self.output.warn('enable llvm_tools for Debug builds')

        if self._is_compile_with_llvm_tools_enabled() and not self._is_llvm_tools_enabled():
            raise ConanInvalidConfiguration("llvm_tools must be enabled")

        if self.options.enable_valgrind:
            self.options["chromium_base"].enable_valgrind = True

        if self.options.enable_ubsan \
           or self.options.enable_asan \
           or self.options.enable_msan \
           or self.options.enable_tsan:
            if not self._is_llvm_tools_enabled():
                raise ConanInvalidConfiguration("sanitizers require llvm_tools")

        if self.options.enable_ubsan \
           or self.options.enable_asan \
           or self.options.enable_msan \
           or self.options.enable_tsan:
            if not self.options["boost"].no_exceptions:
                raise ConanInvalidConfiguration("sanitizers require boost without exceptions")

        if self.options.enable_ubsan:
            self.options["chromium_base"].enable_ubsan = True
            self.options["chromium_libxml"].enable_ubsan = True
            self.options["boost"].enable_ubsan = True
            self.options["corrade"].enable_ubsan = True
            self.options["openssl"].enable_ubsan = True
            if self._is_tests_enabled():
              self.options["conan_gtest"].enable_ubsan = True
              self.options["benchmark"].enable_ubsan = True

        if self.options.enable_asan:
            self.options["chromium_base"].enable_asan = True
            self.options["chromium_libxml"].enable_asan = True
            self.options["boost"].enable_asan = True
            self.options["corrade"].enable_asan = True
            self.options["openssl"].enable_asan = True
            if self._is_tests_enabled():
              self.options["conan_gtest"].enable_asan = True
              self.options["benchmark"].enable_asan = True

        if self.options.enable_msan:
            self.options["chromium_base"].enable_msan = True
            self.options["chromium_libxml"].enable_msan = True
            self.options["boost"].enable_msan = True
            self.options["corrade"].enable_msan = True
            self.options["openssl"].enable_msan = True
            if self._is_tests_enabled():
              self.options["conan_gtest"].enable_msan = True
              self.options["benchmark"].enable_msan = True

        if self.options.enable_tsan:
            self.options["chromium_base"].enable_tsan = True
            self.options["chromium_libxml"].enable_tsan = True
            self.options["boost"].enable_tsan = True
            self.options["corrade"].enable_tsan = True
            self.options["openssl"].enable_tsan = True
            if self._is_tests_enabled():
              self.options["conan_gtest"].enable_tsan = True
              self.options["benchmark"].enable_tsan = True

    def build_requirements(self):
        self.build_requires("cmake_platform_detection/master@conan/stable")
        self.build_requires("cmake_build_options/master@conan/stable")
        self.build_requires("cmake_helper_utils/master@conan/stable")

        if self.options.enable_tsan \
            or self.options.enable_msan \
            or self.options.enable_asan \
            or self.options.enable_ubsan:
          self.build_requires("cmake_sanitizers/master@conan/stable")

        if self._is_cppcheck_enabled():
          self.build_requires("cppcheck_installer/1.90@conan/stable")

        # provides clang-tidy, clang-format, IWYU, scan-build, etc.
        if self._is_llvm_tools_enabled():
          self.build_requires("llvm_tools/master@conan/stable")

        if self._is_tests_enabled():
            self.build_requires("catch2/[>=2.1.0]@bincrafters/stable")
            self.build_requires("FakeIt/[>=2.0.5]@gasuketsu/stable")

    def requirements(self):
        self.requires("boost/1.72.0@dev/stable")

        self.requires("corrade/v2020.06@conan/stable")

        self.requires("fmt/master@dev/stable")

        # \note dispatcher must be thread-safe,
        # so use entt after patch https://github.com/skypjack/entt/issues/449
        # see https://github.com/skypjack/entt/commit/74f3df83dbc9fc4b43b8cfb9d71ba02234bd5c4a
        self.requires("entt/3.5.2")

        self.requires("chromium_build_util/master@conan/stable")
        
        # see use_test_support option in base
        self.requires("chromium_libxml/stable@conan/stable")

        if self._is_tests_enabled():
          # see use_test_support option in base
          self.requires("conan_gtest/stable@conan/stable")
          self.requires("benchmark/v1.5.2@dev/stable")

        no_doctest = (str(self.settings.build_type).lower() != "debug"
          and str(self.settings.build_type).lower() != "relwithdebinfo")
        if not no_doctest:
          self.requires("doctest/[>=2.3.8]")

        #if self.settings.os == "Linux":
        #    self.requires("chromium_dynamic_annotations/master@conan/stable")

        self.requires("chromium_base/master@conan/stable")

        #self.requires("flatbuffers/1.11.0@google/stable")

        #self.requires("flatc_conan/v1.11.0@conan/stable")

        # must match openssl version used in webrtc
        self.requires("openssl/1.1.1-stable@conan/stable")
    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.parallel = True
        cmake.verbose = True

        cmake.definitions["ENABLE_VALGRIND"] = 'ON'
        if not self.options.enable_valgrind:
            cmake.definitions["ENABLE_VALGRIND"] = 'OFF'

        cmake.definitions["ENABLE_UBSAN"] = "ON" if self.options.enable_ubsan else "OFF"

        cmake.definitions["ENABLE_ASAN"] = "ON" if self.options.enable_asan else "OFF"

        cmake.definitions["ENABLE_MSAN"] = "ON" if self.options.enable_msan else "OFF"

        cmake.definitions["ENABLE_TSAN"] = "ON" if self.options.enable_tsan else "OFF"

        cmake.definitions["CONAN_AUTO_INSTALL"] = 'OFF'

        no_doctest = (str(self.settings.build_type).lower() != "debug"
          and str(self.settings.build_type).lower() != "relwithdebinfo")
        if no_doctest:
          cmake.definitions["ENABLE_DOCTEST"] = 'OFF'
          self.output.info('Disabled DOCTEST')
        else:
          cmake.definitions["ENABLE_DOCTEST"] = 'ON'
          self.output.info('Enabled DOCTEST')

        cmake.definitions["CONAN_AUTO_INSTALL"] = 'OFF'
        if self.options.shared:
            cmake.definitions["BUILD_SHARED_LIBS"] = "ON"

        self.add_cmake_option(cmake, "ENABLE_TESTS", self._is_tests_enabled())

        self.add_cmake_option(cmake, "COMPILE_WITH_LLVM_TOOLS", self._is_compile_with_llvm_tools_enabled())

        cmake.configure(build_folder=self._build_subfolder)

        return cmake

    def package(self):
        with tools.vcvars(self.settings, only_diff=False): # https://github.com/conan-io/conan/issues/6577
          self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
          cmake = self._configure_cmake()
          cmake.install()
          # Local build
          # see https://docs.conan.io/en/latest/developing_packages/editable_packages.html
          if not self.in_local_cache:
              self.copy("conanfile.py", dst=".", keep_path=False)

          self.copy_conanfile_for_editable_package(".")

          self.rmdir_if_packaged('.git')
          self.rmdir_if_packaged('tests')
          self.rmdir_if_packaged('lib/tests')
          self.rmdir_if_packaged('lib/pkgconfig')

    def build(self):
        with tools.vcvars(self.settings, only_diff=False): # https://github.com/conan-io/conan/issues/6577
          cmake = self._configure_cmake()
          if self.settings.compiler == 'gcc':
              cmake.definitions["CMAKE_C_COMPILER"] = "gcc-{}".format(
                  self.settings.compiler.version)
              cmake.definitions["CMAKE_CXX_COMPILER"] = "g++-{}".format(
                  self.settings.compiler.version)

          #cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = 'conan_paths.cmake'

          # The CMakeLists.txt file must be in `source_folder`
          cmake.configure(source_folder=".")

          cpu_count = tools.cpu_count()
          self.output.info('Detected %s CPUs' % (cpu_count))

          # -j flag for parallel builds
          cmake.build(args=["--", "-j%s" % cpu_count])

          if self._is_tests_enabled():
            self.output.info('Running tests')
            #cmake.build(args=["--target", "basis_run_all_tests", "--", "-j%s" % cpu_count])
            #self.run('ctest --parallel %s' % (cpu_count))
            cmake.test(target="basis_run_unittests", output_on_failure=True)

    # Importing files copies files from the local store to your project.
    def imports(self):
        dest = os.getenv("CONAN_IMPORT_PATH", "bin")
        self.copy("license*", dst=dest, ignore_case=True)
        self.copy("*.dll", dst=dest, src="bin")
        self.copy("*.so*", dst=dest, src="bin")
        self.copy("*.pdb", dst=dest, src="lib")
        self.copy("*.dylib*", dst=dest, src="lib")
        self.copy("*.lib*", dst=dest, src="lib")
        self.copy("*.a*", dst=dest, src="lib")

    # package_info() method specifies the list of
    # the necessary libraries, defines and flags
    # for different build configurations for the consumers of the package.
    # This is necessary as there is no possible way to extract this information
    # from the CMake install automatically.
    # For instance, you need to specify the lib directories, etc.
    def package_info(self):
        #self.cpp_info.libs = ["basis"]

        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.bindirs = ["bin"]
        self.env_info.LD_LIBRARY_PATH.append(
            os.path.join(self.package_folder, "lib"))
        self.env_info.PATH.append(os.path.join(self.package_folder, "bin"))
        for libpath in self.deps_cpp_info.lib_paths:
            self.env_info.LD_LIBRARY_PATH.append(libpath)

        #self.cpp_info.includedirs.append(os.getcwd())
        #self.cpp_info.includedirs.append(
        #  os.path.join("base", "third_party", "basis"))
        #self.cpp_info.includedirs.append(
        #  os.path.join("base", "third_party", "basis", "compat"))

        #if self.settings.os == "Linux":
        #  self.cpp_info.defines.append('HAVE_CONFIG_H=1')

        # in linux we need to link also with these libs
        #if self.settings.os == "Linux":
        #    self.cpp_info.libs.extend(["pthread", "dl", "rt"])

        #self.cpp_info.libs = tools.collect_libs(self)
        #self.cpp_info.defines.append('PDFLIB_DLL')
