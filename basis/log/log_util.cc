#include "basis/log/log_util.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>

namespace basis {

void initLogging(const std::string& logFile) {
  logging::LoggingSettings logging_settings;
  logging_settings.logging_dest
    = logFile.empty() ?
      logging::LOG_TO_SYSTEM_DEBUG_LOG : logging::LOG_TO_FILE;
  logging_settings.log_file = logFile.c_str();
  // Indicates that the log file should be locked when being written to.
  // Unless there is only one single-threaded process that is logging to
  // the log file, the file should be locked during writes to make each
  // log output atomic. Other writers will block.
  logging_settings.lock_log = logging::LOCK_LOG_FILE;
  CHECK(logging::InitLogging(logging_settings));

  // To view log output with IDs and timestamps use "adb logcat -v threadtime".
  logging::SetLogItems(true,   // Process ID
                       true,   // Thread ID
                       true,   // Timestamp
                       true);  // Tick count
  VLOG(1)
    << "Log file "
    << (logFile.empty()
       ? "not provided"
       : logging_settings.log_file);
}

}  // namespace basis
