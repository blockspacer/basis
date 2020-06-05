#include "Logger.hpp" // IWYU pragma: associated

#include <base/location.h>

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

#include <iostream>

namespace gloer {
namespace log {

bool VlogIsOnForLocation(const base::Location& from_here, int verbose_level) {
  return (verbose_level <=
          logging::GetVlogLevelHelper(from_here.file_name(),
                                      ::strlen(from_here.file_name())));
}

//Logger::Logger() { initLogging(); }
//
//Logger::~Logger() { shutdown(); }
//
//void Logger::initLogging(const std::string& log_file) {
//  /// \note make sure you init-ed CommandLine like so `base::CommandLine::Init(0, nullptr);`
//  logging::LoggingSettings logging_settings;
//  logging_settings.logging_dest
//    = log_file.empty() ?
//      logging::LOG_TO_SYSTEM_DEBUG_LOG : logging::LOG_TO_FILE;
//  logging_settings.log_file = log_file.c_str();
//  // Indicates that the log file should be locked when being written to.
//  // Unless there is only one single-threaded process that is logging to
//  // the log file, the file should be locked during writes to make each
//  // log output atomic. Other writers will block.
//  logging_settings.lock_log = logging::LOCK_LOG_FILE;
//  CHECK(logging::InitLogging(logging_settings));
//
//  // To view log output with IDs and timestamps use "adb logcat -v threadtime".
//  logging::SetLogItems(true,   // Process ID
//                       true,   // Thread ID
//                       true,   // Timestamp
//                       true);  // Tick count
//  VLOG(1)
//    << "Log file "
//    << logging_settings.log_file;
//}
//
//void Logger::shutdown() {
//}

} // namespace log
} // namespace gloer
