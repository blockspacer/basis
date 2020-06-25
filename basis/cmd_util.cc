#include "basis/cmd_util.hpp" // IWYU pragma: associated

#include <base/logging.h>
#include <base/command_line.h>
#include <base/base_switches.h>
#include <base/feature_list.h>
#include <base/strings/string_piece.h>
#include <base/files/file_util.h>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/value_semantic.hpp>

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
  base::CommandLine::Init(argc, argv);

  DCHECK(base::CommandLine::ForCurrentProcess());
  base::CommandLine* command_line
    = base::CommandLine::ForCurrentProcess();

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

int cmdKeyToInt(
  const base::StringPiece &key,
  cmd::BoostCmdParser& boostCmdParser)
{
  CHECK(!key.empty());

  int result = std::numeric_limits<int>::max();

  CHECK(key.find_first_of(',')
        == base::StringPiece::npos);
  if(!boostCmdParser.count(key)) {
    VLOG(9)
        << "Unable to find command-line argument: "
        << key;
    return
      result;
  }

  const boost::optional<int>& value
    = boostCmdParser.getAs<
    boost::optional<int>
    >(key.as_string());

  if(value.is_initialized()) {
    result = value.value();
  } else {
    VLOG_NOT_INITIALIZED(9, key)
  }

  return
    result;
}

base::FilePath getAsPath(
  const base::StringPiece &key,
  cmd::BoostCmdParser& boostCmdParser)
{
  CHECK(!key.empty());

  base::FilePath result{};

  CHECK(key.find_first_of(',')
        == base::StringPiece::npos);
  if(!boostCmdParser.count(key)) {
    VLOG(9)
        << "Unable to find command-line argument: "
        << key;
    return
      result;
  }

  const boost::optional<std::string>& value
    = boostCmdParser.getAs<
    boost::optional<std::string>
    >(key.as_string());

  if(value.is_initialized()
     && !value.value().empty())
  {
    result = base::FilePath{value.value()};
  } else {
    VLOG_NOT_INITIALIZED(9, key)
  }

  return
    result;
}

base::FilePath cmdKeyToDirectory(
  const char key[],
  cmd::BoostCmdParser& boostCmdParser)
{
  base::FilePath dirPath
    = getAsPath(key, boostCmdParser);

  VLOG(9)
    << key
    << " equals to "
    << dirPath;

  if(dirPath.empty()) {
    return
      base::FilePath{};
  }

  /// \note On POSIX, |MakeAbsoluteFilePath| fails
  /// if the path does not exist
  dirPath
    = base::MakeAbsoluteFilePath(dirPath);
  DCHECK(!dirPath.empty())
    << "unable to find absolute path to "
    << dirPath;

  if (!base::PathExists(dirPath)) {
    LOG_PATH_NOT_EXIST(WARNING, dirPath)
    return
      base::FilePath{};
  }

  // we expect dir, NOT file
  if(!base::DirectoryExists(dirPath)) {
    LOG_PATH_NOT_DIRECTORY(WARNING, dirPath)
    return
      base::FilePath{};
  }

  return
    dirPath;
}

base::FilePath cmdKeyToFile(
  const char key[],
  cmd::BoostCmdParser& boostCmdParser)
{
  base::FilePath filePath
    = getAsPath(key, boostCmdParser);

  VLOG(9)
    << key
    << " equals to "
    << filePath;

  if(filePath.empty()) {
    return
      base::FilePath{};
  }

  /// \note On POSIX, |MakeAbsoluteFilePath| fails
  /// if the path does not exist
  filePath = base::MakeAbsoluteFilePath(filePath);
  DCHECK(!filePath.empty())
    << "unable to find absolute path to "
    << filePath;

  if (!base::PathExists(filePath)) {
    LOG_PATH_NOT_EXIST(WARNING, filePath)
    return
      base::FilePath{};
  }

  // we expect file, NOT dir
  if (base::DirectoryExists(filePath)) {
    LOG_PATH_MUST_BE_NOT_DIRECTORY(WARNING, filePath)
    return
      base::FilePath{};
  }

  base::File::Info fileInfo;
  bool hasInfo
    = base::GetFileInfo(filePath, &fileInfo);
  if(!hasInfo) {
    LOG(WARNING)
        << "unable to get source file information: "
        << filePath;
    return
      base::FilePath{};
  }

  return
    filePath;
}

[[nodiscard]] /* do not ignore return value */
std::vector<base::FilePath>
toFilePaths(const std::vector<std::string>& paths)
{
  std::vector<base::FilePath> result;
  for (const std::string& it: paths)
  {
    result.push_back(
      /// \note On POSIX, |MakeAbsoluteFilePath| fails
      /// if the path does not exist
      base::MakeAbsoluteFilePath(base::FilePath{it}));
  }
  return
    result;
}

}  // namespace basis
