# About

`basis` extends `base` library from chromium https://github.com/chromium/chromium/tree/master/base

Unlike `base`, `basis` can use third-party libs (`boost`, `beast`, `corrade`, `entt`, etc.)

If you want to add file into `basis` that depends on same third-party libs as `base`,
than prefer to add it into `basic` (`chromium_base_conan/extensions/basic/`) instead of `basis`.

`basis` is pulled into many projects.

So the bar for adding stuff is that it must have demonstrated wide applicability.

Pull into basis only when needed.

In a project our size, sometimes even duplication is OK and inevitable.

## Before build (dependencies)

Create clang12_compiler conan profile https://docs.conan.io/en/1.34/reference/profiles.html#examples

Re-build dependencies:

```bash
git clone https://github.com/blockspacer/conan_github_downloader.git ~/conan_github_downloader

cmake \
  -DSCRIPT_PATH="$PWD/get_conan_dependencies.cmake"\
  -DEXTRA_CONAN_OPTS="--profile;clang12_compiler\
;-s;build_type=Debug\
;-s;cling_conan:build_type=Release\
;-s;llvm_tools:build_type=Release\
;--build;missing" \
  -P ~/conan_github_downloader/conan_github_downloader.cmake
```

## Build

```bash
export VERBOSE=1
export CONAN_REVISIONS_ENABLED=1
export CONAN_VERBOSE_TRACEBACK=1
export CONAN_PRINT_RUN_COMMANDS=1
export CONAN_LOGGING_LEVEL=10

# NOTE: change `build_type=Debug` to `build_type=Release` in production
conan create . \
  conan/stable \
  -s build_type=Debug \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
  -e basis:enable_tests=True \
  -o openssl:shared=True \
  -o chromium_base:shared=True \
  -e chromium_base:enable_tests=True \
  -o perfetto:is_hermetic_clang=False \
  --build missing \
  --build cascade

# clean build cache
conan remove "*" --build --force
```

## Dev-only build (local conan flow)

```bash
find . -type f -name "*_buildflags.h" -exec rm {} \;
find . -type f -name "*_buildflags.tmp" -exec rm {} \;

export PKG_NAME=basis/master@conan/stable

(CONAN_REVISIONS_ENABLED=1 \
    conan remove --force $PKG_NAME || true)

(rm -rf local_build || true)

export CONAN_REVISIONS_ENABLED=1
export CONAN_VERBOSE_TRACEBACK=1
export CONAN_PRINT_RUN_COMMANDS=1
export CONAN_LOGGING_LEVEL=10

# NOTE: use --build=missing if you got error `ERROR: Missing prebuilt package`
cmake -E time \
  conan install . \
  --install-folder local_build \
  -s build_type=Debug \
  -s cling_conan:build_type=Release \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
  -e basis:enable_tests=True \
  -o basis:shared=False \
  -o chromium_base:shared=True \
  -e chromium_base:enable_tests=True \
  -o perfetto:is_hermetic_clang=False

(rm local_build/CMakeCache.txt || true)

cmake -E time \
  conan source . \
  --source-folder . \
  --install-folder local_build

# You can use `cmake --build . -- -j14` on second run.
cmake -E time \
  conan build . \
  --build-folder local_build \
  --source-folder . \
  --install-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir \
  --source-folder . \
  --install-folder local_build

cmake -E time \
  conan export-pkg . conan/stable \
  --package-folder local_build/package_dir \
  --force \
  -s build_type=Debug \
  -s cling_conan:build_type=Release \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
  -e basis:enable_tests=True \
  -o basis:shared=False \
  -o chromium_base:shared=True \
  -e chromium_base:enable_tests=True \
  -o perfetto:is_hermetic_clang=False

cmake -E time \
  conan test test_package basis/master@conan/stable \
  -s build_type=Debug \
  -s cling_conan:build_type=Release \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
  -e basis:enable_tests=True \
  -o basis:shared=False \
  -o chromium_base:shared=True \
  -e chromium_base:enable_tests=True \
  -o perfetto:is_hermetic_clang=False
```

## HOW TO BUILD WITH SANITIZERS ENABLED

Use `enable_asan` or `enable_ubsan`, etc.

```bash
export VERBOSE=1
export CONAN_REVISIONS_ENABLED=1
export CONAN_VERBOSE_TRACEBACK=1
export CONAN_PRINT_RUN_COMMANDS=1
export CONAN_LOGGING_LEVEL=10

# NOTE: change `build_type=Debug` to `build_type=Release` in production
conan create . \
    conan/stable \
    -s build_type=Debug \
    -s llvm_tools:build_type=Release \
    -o llvm_tools:enable_tsan=True \
    -o llvm_tools:include_what_you_use=False \
    --profile clang12_compiler \
    --build chromium_base \
    --build chromium_tcmalloc \
    -e chromium_base:enable_tests=True \
    -o chromium_base:enable_tsan=True \
    -e chromium_base:enable_llvm_tools=True \
    -o chromium_base:use_alloc_shim=False \
    -e basis:enable_tests=True \
    -o basis:enable_tsan=True \
    -e basis:enable_llvm_tools=True \
    -o chromium_tcmalloc:use_alloc_shim=False \
    -o openssl:shared=True

# clean build cache
conan remove "*" --build --force
```

## For contibutors: conan editable mode

With the editable packages, you can tell Conan where to find the headers and the artifacts ready for consumption in your local working directory.
There is no need to run `conan create` or `conan export-pkg`.

See for details [https://docs.conan.io/en/latest/developing_packages/editable_packages.html](https://docs.conan.io/en/latest/developing_packages/editable_packages.html)

Build locally:

```bash
export VERBOSE=1
export CONAN_REVISIONS_ENABLED=1
export CONAN_VERBOSE_TRACEBACK=1
export CONAN_PRINT_RUN_COMMANDS=1
export CONAN_LOGGING_LEVEL=10

export PKG_NAME=basis/master@conan/stable

(CONAN_REVISIONS_ENABLED=1 \
    conan remove --force $PKG_NAME || true)

cmake -E time \
  conan install . \
  --install-folder local_build \
  -s build_type=Debug \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
      --build missing \
      --build cascade \
      -e chromium_base:enable_tests=True \
      -e chromium_base:enable_llvm_tools=True \
      -e basis:enable_tests=True \
      -e basis:enable_llvm_tools=True \
      -o openssl:shared=True

cmake -E time \
  conan source . \
  --source-folder local_build \
  --install-folder local_build

conan build . \
  --build-folder local_build \
  --source-folder local_build \
  --install-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir \
  --source-folder local_build \
  --install-folder local_build
```

Set package to editable mode:

```bash
conan editable add local_build/package_dir \
  basis/master@conan/stable
```

Note that `conanfile.py` modified to detect local builds via `self.in_local_cache`

After change source in folder local_build (run commands in source package folder):

```
conan build . \
  --build-folder local_build \
  --source-folder local_build \
  --install-folder local_build

conan package . \
  --build-folder local_build \
  --package-folder local_build/package_dir \
  --source-folder local_build \
  --install-folder local_build
```

Build your test project

In order to revert the editable mode just remove the link using:

```bash
conan editable remove \
  basis/master@conan/stable
```

## For contibutors: valgrind

Make sure you use `Debug` build with `-e basis:enable_llvm_tools=True`

See for details:

* [https://www.jetbrains.com/help/clion/memory-profiling-with-valgrind.html](https://www.jetbrains.com/help/clion/memory-profiling-with-valgrind.html)
* [https://heeris.id.au/2016/valgrind-gdb/](https://heeris.id.au/2016/valgrind-gdb/)

Install valgrind:

```bash
sudo apt install valgrind  # Ubuntu, Debian, etc.
# OR
sudo yum install valgrind  # RHEL, CentOS, Fedora, etc.
```

NOTE: make sure you set `use_alloc_shim=False` and `enable_valgrind=True` (see below)

Run valgrind via cmake:

```bash
export VERBOSE=1
export CONAN_REVISIONS_ENABLED=1
export CONAN_VERBOSE_TRACEBACK=1
export CONAN_PRINT_RUN_COMMANDS=1
export CONAN_LOGGING_LEVEL=10

export PKG_NAME=basis/master@conan/stable

(CONAN_REVISIONS_ENABLED=1 \
    conan remove --force $PKG_NAME || true)

# NOTE: set `use_alloc_shim=False` and `enable_valgrind=True` for valgrind support
cmake -E time \
  conan install . \
  --install-folder local_build \
  -s build_type=Debug \
  -s cling_conan:build_type=Release \
  -s llvm_tools:build_type=Release \
  --profile clang12_compiler \
  -o basis:enable_valgrind=True \
  -e basis:enable_tests=True \
  -e basis:enable_llvm_tools=True \
  -o chromium_base:enable_valgrind=True \
  -e chromium_base:enable_llvm_tools=True \
  -o chromium_base:use_alloc_shim=False \
  -o chromium_tcmalloc:use_alloc_shim=False \
  --build chromium_base \
  --build chromium_tcmalloc

cd ~/basis

# see section about `conan editable mode`
cd local_build

# remove old CMakeCache
(rm CMakeCache.txt || true)

# remove old build artifacts
rm -rf basis
rm -rf bin
find . -iname '*.o' -exec rm {} \;
find . -iname '*.a' -exec rm {} \;
find . -iname '*.dll' -exec rm {} \;
find . -iname '*.lib' -exec rm {} \;

# NOTE: -DENABLE_VALGRIND=ON
cmake .. \
  -DENABLE_VALGRIND=ON \
  -DENABLE_TESTS=TRUE \
  -DENABLE_DOCTEST=ON \
  -DENABLE_VALGRIND_TESTS=TRUE \
  -DBUILD_SHARED_LIBS=FALSE \
  -DCONAN_AUTO_INSTALL=OFF \
  -DCMAKE_BUILD_TYPE=Debug

# NOTE: to run some tests under valgrind
# use `-DENABLE_VALGRIND_TESTS=TRUE`
cmake -E time cmake --build . \
  --target basis-fakeit_run_valgrind

# you can run some tests to check valgrind
# TODO: provide options to valgrind
# valgrind ./local_build/bin/Debug/tests/basis-gmock \
#   --gtest_filter=ToolsSanityTest.DISABLED_ValgrindTest \
#   --gtest_also_run_disabled_tests

# search for valgrind log file
find $PWD -name *valgrind*.log
```

TODO: try to build with modern clang https://stackoverflow.com/questions/40509986/valgrind-reporting-mismatched-free-delete-delete

TODO: valgrind may not support chromium base, FIXME. And remove GTEST_NO_SUITE

NOTE: you can add valgrind suppressions in `cmake/valgrind.cfg`

NOTE: compile program with a debug flag to run under valgrind

NOTE: use `valgrind --tool=helgrind` to detect potential deadlocks and data races

NOTE: use `valgrind --tool=massif --massif-out-file=massif_file --stacks=true` to measure size of heap. See also https://kde.org/applications/development/org.kde.massif-visualizer

See for details https://stackoverflow.com/a/44989219

## Qualifications for being in basis OWNERS

interest and ability to learn low level/high detail/complex c++ stuff

inclination to always ask why and understand everything (including external interactions like win32) rather than just hoping the author did it right
mentorship/experience

demonstrated good judgement (esp with regards to public APIs) over a length of time

Owners are added when a contributor has shown the above qualifications and when they express interest.

There isn't an upper bound on the number of OWNERS.

## Design and naming

Be sure to use the basis namespace.

STL-like constructs should adhere as closely to STL as possible.

Functions and behaviors not present in STL should only be added when they are related to the specific data structure implemented by the container.

For STL-like constructs our policy is that they should use STL-like naming even when it may conflict with the style guide.

So functions and class names should be lower case with underscores.

## Disclaimer

That open source project based on the Google Chromium project.

This is not official Google product.

Portions Copyright (c) Google Inc.

See LICENSE files.
