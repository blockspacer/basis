#include "basis/cmd_util.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/command_line.h>
#include <base/base_switches.h>
#include <base/feature_list.h>
#include <base/strings/string_piece.h>
#include <base/files/file_util.h>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include <limits>

#define LOG_PATH_NOT_DIRECTORY(severity, path) \
  LOG(severity) \
    << "path must be directory: " \
    << path;

#define LOG_PATH_MUST_BE_NOT_DIRECTORY(severity, path) \
  LOG(severity) \
    << "path must be NOT directory: " \
    << path;

#define VLOG_NOT_INITIALIZED(severity, key) \
  VLOG(severity) \
    << "command like argument " \
    << key \
    << " is not initialized";

#define LOG_PATH_NOT_EXIST(severity, path) \
  LOG(severity) \
    << "path must exist: " \
    << path;

namespace basis {

// extern
const char DEFAULT_EVENT_CATEGORIES[]
  = "-sequence_manager"
    ",-thread_pool"
    ",-base"
    ",-toplevel"
    ",profiler"
    ",user_timing"
    ",ui"
    ",browser"
    ",latency"
    ",latencyInfo"
    ",loading"
    ",skia"
    ",task_scheduler"
    ",native"
    ",benchmark"
    ",ipc"
    ",mojom"
    ",media"
    ",disabled-by-default-lifecycles"
    ",disabled-by-default-renderer.scheduler"
    ",disabled-by-default-v8.gc"
    ",disabled-by-default-blink_gc"
    ",disabled-by-default-system_stats"
    ",disabled-by-default-network"
    ",disabled-by-default-cpu_profiler"
    ",disabled-by-default-memory-infra";

void initCommandLine(int argc, char* argv[])
{
  // see https://peter.sh/experiments/chromium-command-line-switches/
  DCHECK(!base::CommandLine::InitializedForCurrentProcess());
  ::base::CommandLine::Init(argc, argv);

  DCHECK(base::CommandLine::ForCurrentProcess());
  ::base::CommandLine* command_line
    = ::base::CommandLine::ForCurrentProcess();

  // sets default command-line switches
  // initialize |g_vlog_info| i.e. |switches::kV| in debug mode
#if !defined(NDEBUG)
  /// \note usage
  /// ./app --vmodule=*main*=100 --enable-logging=stderr --log-level=100
  // Gives the default maximal active V-logging level; 0 is the default.
  // Normally positive values are used for V-logging levels.
  if(!command_line->HasSwitch(switches::kV)) {
    command_line->AppendSwitchASCII(switches::kV,
      "1");
    VLOG(9) << "found switch for V-logging level";
  }
  // Gives the per-module maximal V-logging levels to override the value
  // given by --v.  E.g. "my_module=2,foo*=3" would change the logging
  // level for all code in source files "my_module.*" and "foo*.*"
  // ("-inl" suffixes are also disregarded for this matching).
  //
  // Any pattern containing a forward or backward slash will be tested
  // against the whole pathname and not just the module.  E.g.,
  // "*/foo/bar/*=2" would change the logging level for all code in
  // source files under a "foo/bar" directory.
  if(!command_line->HasSwitch(switches::kVModule)) {
    command_line->AppendSwitchASCII(switches::kVModule,
      "*main*=0"
      ",*webrtc*=2"
      ",*libjingle*=2");
    VLOG(9) << "found switch for V-Module";
  }

  // Indicates that crash reporting should be enabled. On platforms where helper
  // processes cannot access to files needed to make this decision, this flag is
  // generated internally.
  if(!command_line->HasSwitch(switches::kEnableCrashReporter)) {
    command_line->AppendSwitchASCII(switches::kEnableCrashReporter,
      "1");
    VLOG(9) << "found switch for Crash-Reporter";
  }

  // Generates full memory crash dump.
  if(!command_line->HasSwitch(switches::kFullMemoryCrashReport)) {
    command_line->AppendSwitchASCII(switches::kFullMemoryCrashReport,
      "1");
    VLOG(9) << "found switch for Memory-Crash-Report";
  }

#if defined(OS_LINUX)
    // Controls whether or not retired instruction counts are surfaced for threads
    // in trace events on Linux.
    //
    // This flag requires the BPF sandbox to be disabled.  if(!command_line->HasSwitch(switches::kVModule)) {
    if(!command_line->HasSwitch(switches::kEnableThreadInstructionCount)) {
      command_line->AppendSwitchASCII(switches::kEnableThreadInstructionCount,
        "1");
      VLOG(9) << "found switch for Thread-Instruction-Count";
    }
#endif

  DCHECK(command_line->HasSwitch(switches::kV) ||
      command_line->HasSwitch(switches::kVModule));
#endif // NDEBUG
}

}  // namespace basis
