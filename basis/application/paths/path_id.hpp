#pragma once

namespace application {

namespace paths {

// Enumeration of special paths that the platform can define.
typedef enum AppPathId {
  // Path to where the local content files that ship with the binary are
  // available.
  kAppPathContentDirectory,

  // Path to the directory that can be used as a local file cache, if
  // available.
  kAppPathCacheDirectory,

  // Path to the directory where debug output (e.g. logs, trace output,
  // screenshots) can be written into.
  kAppPathDebugOutputDirectory,

  // Path to a directory where temporary files can be written.
  kAppPathTempDirectory,

  // Path to a directory where test results can be written.
  kAppPathTestOutputDirectory,

  // Full path to the executable file.
  kAppPathExecutableFile,
} AppPathId;

} // namespace paths

} // namespace application
