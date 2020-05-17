from conans import ConanFile, CMake, tools, python_requires
import traceback
import os
import shutil

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

class basis_conan_project(conan_build_helper.CMakePackage):
    name = "basis"

    # Indicates License type of the packaged library
    # TODO (!!!)
    # license = "MIT"

    version = "master"

    # TODO (!!!)
    #url = "https://github.com/blockspacer/CXXCTP"

    description = "`basis` library"
    topics = ('c++')

    options = {
        "shared": [True, False],
        "debug": [True, False],
        "enable_sanitizers": [True, False]
    }

    default_options = (
        "shared=False",
        "debug=False",
        "enable_sanitizers=False",
        "*:integration=catch", # for FakeIt,
        # chromium_base
        "chromium_base:use_alloc_shim=True",
        # chromium_tcmalloc
        "chromium_tcmalloc:use_alloc_shim=True",
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
    exports_sources = ("LICENSE", "*.md", "include/*", "src/*",
                       "cmake/*", "CMakeLists.txt", "tests/*", "benchmarks/*",
                       "scripts/*", "tools/*", "codegen/*", "assets/*",
                       "docs/*", "licenses/*", "patches/*", "resources/*",
                       "submodules/*", "thirdparty/*", "third-party/*",
                       "third_party/*", "base/*", "build/*", "basis/*")

    settings = "os", "compiler", "build_type", "arch"

    #def source(self):
    #  url = "https://github.com/....."
    #  self.run("git clone %s ......." % url)

    def build_requirements(self):
        self.build_requires("cmake_platform_detection/master@conan/stable")
        self.build_requires("cmake_build_options/master@conan/stable")

        if self._is_tests_enabled():
            self.build_requires("catch2/[>=2.1.0]@bincrafters/stable")
            self.build_requires("FakeIt/[>=2.0.5]@gasuketsu/stable")

    def requirements(self):
        self.requires("chromium_build_util/master@conan/stable")
        
        # see use_test_support option in base
        self.requires("chromium_libxml/master@conan/stable")

        # see use_test_support option in base
        self.requires("gtest/[>=1.8.0]@bincrafters/stable")

        #if self.settings.os == "Linux":
        #    self.requires("chromium_dynamic_annotations/master@conan/stable")

        self.requires("chromium_base/master@conan/stable")

        self.requires("corrade/2019.10@magnum/stable")

        self.requires("flatbuffers/1.11.0@google/stable")

        #self.requires("flatc_conan/v1.11.0@conan/stable")

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.parallel = True
        cmake.verbose = True

        cmake.definitions["CONAN_AUTO_INSTALL"] = 'OFF'

        if self.options.shared:
            cmake.definitions["BUILD_SHARED_LIBS"] = "ON"

        self.add_cmake_option(cmake, "ENABLE_TESTS", self._is_tests_enabled())

        self.add_cmake_option(cmake, "ENABLE_SANITIZERS", self.options.enable_sanitizers)

        cmake.configure(build_folder=self._build_subfolder)

        return cmake

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()
        # Local build
        # see https://docs.conan.io/en/latest/developing_packages/editable_packages.html
        if not self.in_local_cache:
            self.copy("conanfile.py", dst=".", keep_path=False)

    def build(self):
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
          cmake.build(args=["--target", "basis_run_all_tests", "--", "-j%s" % cpu_count])
          #self.run('ctest --parallel %s' % (cpu_count))
          # TODO: use cmake.test()

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
