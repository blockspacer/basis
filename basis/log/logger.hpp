#pragma once

#include <memory>
#include <string>

#include <base/logging.h>
#include <base/trace_event/trace_event.h>

namespace base {
class Location;
}  // namespace base

#define VLOG_LOC_STREAM(from_here, verbose_level)                     \
  logging::LogMessage(from_here.file_name(), from_here.line_number(), \
                      -verbose_level)                                 \
      .stream()

#define VLOG_LOC_IF(from_here, verbose_level, condition) \
  LAZY_STREAM(VLOG_LOC_STREAM(from_here, verbose_level), \
              condition &&                               \
                  (VLOG_IS_ON(verbose_level) ||          \
                   ::gloer::log::VlogIsOnForLocation(from_here, verbose_level)))

// USAGE:
// command_line->AppendSwitchASCII(switches::kV, "1");
// DVLOG(1) << "number of arguments: " << argc;
//
#define DVLOG_LOC(from_here, verbose_level)              \
  VLOG_LOC_IF(from_here, verbose_level, DCHECK_IS_ON())

#define VLOG_LOC(from_here, verbose_level)              \
  VLOG_LOC_IF(from_here, verbose_level, 1)

namespace gloer {
namespace log {

///**
// * Supported log levels: DEBUG, INFO, WARNING, FATAL
// *
// * Example usage:
// *
// * @code{.cpp}
// * LOG(WARNING) << "This log " << "call";
// * @endcode
// **/
//class Logger {
//public:
//  Logger();
//
//  //Logger(bool enableConsoleSink, bool enableFileSink);
//
//  ~Logger();
//
//  void shutdown();
//
//  void initLogging(const std::string& log_file = "");
//
//private:
//  //bool enableConsoleSink_ = true;
//
//  //bool enableFileSink_ = true;
//
//  std::string log_prefix_ = "server";
//
//  std::string log_directory_ = "";
//
//  std::string log_default_id_ = "";
//};

bool VlogIsOnForLocation(const base::Location& from_here, int verbose_level);

} // namespace log
} // namespace gloer
