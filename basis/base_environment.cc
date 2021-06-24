#include "basis/base_environment.h" // IWYU pragma: associated
#include "basis/path_provider.h"
#include <basis/i18n/i18n.h>
#include <basis/i18n/icu_util.h>
#include <basis/threading/thread_pool_util.h>

#include <base/notreached.h>
#include "base/allocator/allocator_check.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/base_switches.h"
#include "base/threading/hang_watcher.h"
#include "base/command_line.h"
#include "base/sequence_checker.h"
#include "base/process/memory.h"
#include "base/debug/leak_annotations.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/base_paths.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/metrics/statistics_recorder.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/sampling_heap_profiler/sampling_heap_profiler.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_piece_forward.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include <base/task/thread_pool.h>
#include "base/threading/platform_thread.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_buffer.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_log.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"

#include "basic/rvalue_cast.h"
#include "basic/macros.h"
#include <basic/cmd_util.h>
#include <basic/log/log_util.h>
#include <basic/tracing/tracing_util.h>
#include <basic/multiconfig/multiconfig.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <locale>
#include <sstream>
#include <string.h>
#include <locale.h>

namespace basis {

static std::unique_ptr<base::FieldTrialList> setUpFieldTrials() {
  std::unique_ptr<base::FieldTrialList> field_trial_list;
  if (!base::FieldTrialList::GetInstance())
    field_trial_list = std::make_unique<base::FieldTrialList>(nullptr);
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  // Ensure any field trials specified on the command line are initialized.
  if (command_line->HasSwitch(::switches::kForceFieldTrials)) {
    // Create field trials without activating them, so that this behaves in a
    // consistent manner with field trials created from the server.
    bool result = base::FieldTrialList::CreateTrialsFromString(
        command_line->GetSwitchValueASCII(::switches::kForceFieldTrials));
    CHECK(result) << "Invalid --" << ::switches::kForceFieldTrials
                  << " list specified.";
  }

  return field_trial_list;
}

ScopedBaseEnvironment::ScopedBaseEnvironment()
  : main_loop_task_runner(base::ThreadTaskRunnerHandle::Get())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(base::ThreadTaskRunnerHandle::Get());
}

ScopedBaseEnvironment::~ScopedBaseEnvironment()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  {
    DCHECK(base::trace_event::MemoryDumpManager::GetInstance());
    ::base::trace_event::MemoryDumpManager::GetInstance()
      ->TeardownForTracing();
  }

  // save tracing report to file, if needed
  {
    const bool need_write_tracing_report
      = ::base::trace_event::TraceLog::GetInstance()->IsEnabled();
    if(need_write_tracing_report) {
      DCHECK(traceReportPath_);
      ::basic::writeTraceReport(
        *traceReportPath_);
    } else {
      DVLOG(9)
        << "tracing disabled";
    }
  }

  {
    DVLOG(9)
      << "ThreadPool Shutdown...";
    TRACE_EVENT0("shutdown", "MainLoop:ThreadPool");
    DCHECK(base::ThreadTaskRunnerHandle::Get());
    base::ThreadPoolInstance::Get()->Shutdown();
    // Tasks posted with TaskShutdownBehavior::BLOCK_SHUTDOWN and
    // tasks posted with TaskShutdownBehavior::SKIP_ON_SHUTDOWN that
    // have started to run before the Shutdown() call
    // have now completed their execution.
    // Tasks posted with TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN
    // may still be running.
  }
}

bool ScopedBaseEnvironment::init(
  int argc
  , char* argv[]
  , const bool need_auto_start_tracer
  , const std::string event_categories
  , const ::base::FilePath& outDir
  , const ::base::FilePath::CharType icuFileName[]
  , const ::base::FilePath::CharType traceReportFileName[]
  , const int threadsNum)
{
  CHECK(setlocale(LC_ALL, "en_US.UTF-8") != nullptr)
      << "Failed to set locale: " << "en_US.UTF-8";

  // Various things break when you're using a locale where the decimal
  // separator isn't a period.
  CHECK(setlocale(LC_NUMERIC, "C") != nullptr)
      << "Failed to set locale: " << "LC_NUMERIC C";

  if (!base::PathService::Get(base::DIR_EXE, &dir_exe_)) {
    NOTREACHED();
    // stop app execution with EXIT_FAILURE
    return
      false;
  }

  traceReportPath_
    = std::make_unique<const ::base::FilePath>(
        dir_exe_.Append(traceReportFileName));

  /// \note log all command-line arguments before
  /// parsing them as program options
  {
    VLOG(9)
        << "started "
        << dir_exe_
        << " with arguments:";
    for(int i = 0; i < argc; ++i) {
      VLOG(9)
        << " "
        << argv[i]
        << " ";
    }
  }

  ::base::PlatformThread::SetName("Main");

  // see https://stackoverflow.com/a/18981514/10904212
  std::locale::global(std::locale::classic());

  ::basic::initCommandLine(argc, argv);

#if DCHECK_IS_ON()
  // Must be done before hooking any functions that make stack traces.
  ::base::debug::EnableInProcessStackDumping();
#endif // DCHECK_IS_ON()

  ::base::SamplingHeapProfiler::Get()->SetRecordThreadNames(true);

  /// \todo
  // init allocator https://github.com/aranajhonny/chromium/blob/caf5bcb822f79b8997720e589334266551a50a13/content/app/content_main_runner.cc#L512
  // If we are on a platform where the default allocator is overridden (shim
  // layer on windows, tcmalloc on Linux Desktop) smoke-tests that the
  // overriding logic is working correctly. If not causes a hard crash, as its
  // unexpected absence has security implications.
  CHECK(base::allocator::IsAllocatorInitialized());

  // Enables 'terminate on heap corruption' flag. Helps protect against heap
  // overflow. Has no effect if the OS doesn't provide the necessary facility.
  /// \note On Linux, there nothing to do AFAIK.
  ::base::EnableTerminationOnHeapCorruption();

#if DCHECK_IS_ON()
  // Turns on process termination if memory runs out.
  ::base::EnableTerminationOnOutOfMemory();
#endif // DCHECK_IS_ON()

  std::unique_ptr<::base::FeatureList> feature_list
    = std::make_unique<::base::FeatureList>();

  // configure |base::FeatureList|
  {
    std::string default_enable_features
      = ::base::JoinString({}, ",");

    std::string default_disable_features =
      ::base::JoinString({}, ",");

    ::base::CommandLine* command_line
      = ::base::CommandLine::ForCurrentProcess();

    /// \usage --enable-features=console_terminal,remote_console
    std::string cmd_enabled =
        command_line->GetSwitchValueASCII(
          switches::kEnableFeatures);
    if(cmd_enabled.empty()) {
      cmd_enabled = default_enable_features;
    }

    /// \usage --disable-features=console_terminal,remote_console
    std::string cmd_disabled =
        command_line->GetSwitchValueASCII(
          switches::kDisableFeatures);
    if(cmd_disabled.empty()) {
      cmd_disabled = default_disable_features;
    }

    // Initialize the FeatureList from the command line.
    feature_list->InitializeFromCommandLine(cmd_enabled, cmd_disabled);

    /// \note you can override features like so:
    /// feature_list
    ///   ->RegisterOverride(kFeatureConsoleTerminalName
    ///       , ::base::FeatureList::OVERRIDE_ENABLE_FEATURE
    ///       , nullptr // field trial
    /// );
  }

  ::base::FeatureList::SetInstance(RVALUE_CAST(feature_list));

  ::base::FilePath file_exe_{};

  if (!base::PathService::Get(base::FILE_EXE, &file_exe_)) {
    NOTREACHED();
    // stop app execution with EXIT_FAILURE
    return
      false;
  }

  /// \note returns empty string if the path is not ASCII.
  const std::string maybe_base_exe_name_
    = file_exe_.BaseName().RemoveExtension().MaybeAsASCII();

  base::FilePath tmp_dir;
  CHECK(GetTempDir(&tmp_dir));

  MULTICONF_String(log_file_conf
    , /* default value */ tmp_dir.AppendASCII(maybe_base_exe_name_ + ".log").MaybeAsASCII()
    , BUILTIN_MULTICONF_LOADERS
    , /* name of configuration group */ maybe_base_exe_name_);

  /// \note will cache configuration values,
  /// so use `resetAndReload` if you need to update configuration values.
  CHECK_OK(basic::MultiConf::GetInstance().init())
    << "Wrong configuration.";
  /// \note required to refresh configuration cache
  base::RunLoop().RunUntilIdle();

  {
    const std::string& log_file = log_file_conf.GetValue();
    LOG(INFO)
      << "You can"
      << (log_file.empty() ? " set" : " change")
      << " path to log file using configuration option: "
      << log_file_conf.optionFormatted()
      << (log_file.empty() ? "" : " Using path to log file: ")
      << log_file;
    ::basic::initLogging(
      log_file
    );
  }

  DCHECK(!base::FieldTrialList::GetInstance());

#if DCHECK_IS_ON()
  ::base::FieldTrial::EnableBenchmarking();
#endif // DCHECK_IS_ON()

  // This is intentionally leaked since it needs to live for the duration
  // of the process and there's no benefit in cleaning it up at exit.
  base::FieldTrialList* leaked_field_trial_list =
      setUpFieldTrials().release();
  ANNOTATE_LEAKING_OBJECT_PTR(leaked_field_trial_list);
  ignore_result(leaked_field_trial_list);

  // The hang watcher needs to be started once the feature list is available
  // but before the IO thread is started.
  if (base::HangWatcher::IsEnabled()) {
    hang_watcher_ = new base::HangWatcher();
    hang_watcher_->Start();
    ANNOTATE_LEAKING_OBJECT_PTR(hang_watcher_);
  }

  base::PowerMonitor::Initialize(
      std::make_unique<base::PowerMonitorDeviceSource>());

  if(!base::PathExists(dir_exe_.Append(icuFileName))) {
    LOG(ERROR)
        << "unable to load icu i18n data file: "
        << dir_exe_.Append(icuFileName);
    // stop app execution with EXIT_FAILURE
    return
      false;
  }

  ::basis::initICUi18n(icuFileName);

  /// \note you must init ICU before i18n
  i18n = std::make_unique<i18n::I18n>(
    nullptr // locale
    );

  // see |base::RecommendedMaxNumberOfThreadsInThreadGroup|
  {
    const int num_cores
      = ::base::SysInfo::NumberOfProcessors();
    const int kMaxByDemandWorkerThreadsInPool
    /// \note based on command-line paramater
      = 1 + threadsNum;
    const int kForegroundMaxThreads
      = std::max(
          kMaxByDemandWorkerThreadsInPool
          , num_cores - 1);
    CHECK(kForegroundMaxThreads >= 1)
      << "Unable to register foreground threads."
      " Make sure you have at leat one cpu core";

    ::basis::initThreadPool(
      kForegroundMaxThreads);
  }

  // register ::basis::ApplicationPathKeys
  ::basis::AddPathProvider();

  // see http://dev.chromium.org/developers/how-tos/trace-event-profiling-tool
  ::basic::initTracing(
    need_auto_start_tracer
    , event_categories
    );

  /// \todo Disable MemoryPressureListener when memory coordinator is enabled.
  //base::MemoryPressureListener::SetNotificationsSuppressed(false);

  /// \todo send metrics to server,
  /// see https://github.com/chromium/chromium/blob/master/components/metrics/url_constants.cc

  // Initializes StatisticsRecorder which tracks UMA histograms.
  // A histogram is a chart that groups numeric data into bins,
  // displaying the bins as segmented columns.
  // see https://chromium.googlesource.com/chromium/src.git/+/HEAD/tools/metrics/histograms/README.md
  // see https://chromium.googlesource.com/chromium/src/+/lkgr/docs/speed/README.md
  // see https://developers.google.com/chart/interactive/docs/gallery/histogram
  // |InitLogOnShutdown| initializes logging histograms with --v=1.
  // Safe to call multiple times.
  // Is called from ctor but for it seems that it is more useful to
  // start logging after statistics recorder, so we need to init log-on-shutdown
  // later.
  // Usage examples:
  // UMA_HISTOGRAM_BOOLEAN("App.BoolTest()", false);
  // UMA_HISTOGRAM_COUNTS_100("App.TestCounts", 11);
  // UMA_HISTOGRAM_LONG_TIMES("App.TimeNow()", ::base::TimeDelta::FromMinutes(5));
  // UMA_HISTOGRAM_ENUMERATION("Login", OFFLINE_AND_ONLINE, NUM_SUCCESS_REASONS);
  // ::base::UmaHistogramMemoryLargeMB("HeapProfiler.Malloc", malloc_usage_mb);
  ::base::StatisticsRecorder::InitLogOnShutdown();

  // set current path
  {
    CHECK(!outDir.empty());
    ::base::SetCurrentDirectory(outDir);
    ::base::FilePath current_path;
    const bool curDirOk =
      ::base::GetCurrentDirectory(&current_path);
    DCHECK(curDirOk);
    VLOG(9)
        << "Current path is "
        << current_path;
  }

  return
    true;
}

} // basis
