#include "basis/base_environment.hpp" // IWYU pragma: associated

#include "basis/path_provider.hpp"

#include <entt/signal/dispatcher.hpp>

#include <basis/cmd_util.hpp>
#include <basis/i18n.hpp>
#include <basis/icu_util.hpp>
#include <basis/log_util.hpp>
#include <basis/thread_pool_util.hpp>
#include <basis/tracing_util.hpp>

#include <base/process/memory.h>
#include <base/base_paths.h>
#include <base/bind.h>
#include <base/callback_forward.h>
#include <base/debug/stack_trace.h>
#include <base/feature_list.h>
#include <base/files/file.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/memory/ref_counted_memory.h>
#include <base/memory/scoped_refptr.h>
#include <base/message_loop/message_loop.h>
#include <base/message_loop/message_loop_current.h>
#include <base/metrics/statistics_recorder.h>
#include <base/path_service.h>
#include <base/run_loop.h>
#include <base/sampling_heap_profiler/sampling_heap_profiler.h>
#include <base/single_thread_task_runner.h>
#include <base/strings/string_piece.h>
#include <base/strings/string_piece_forward.h>
#include <base/strings/string_util.h>
#include <base/system/sys_info.h>
#include <base/task/thread_pool/thread_pool.h>
#include <base/threading/platform_thread.h>
#include <base/trace_event/memory_dump_manager.h>
#include <base/trace_event/trace_buffer.h>
#include <base/trace_event/trace_event.h>
#include <base/trace_event/trace_log.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <locale>
#include <sstream>
#include <string.h>

namespace basis {

ScopedBaseEnvironment::ScopedBaseEnvironment()
  : main_loop_task_runner(base::MessageLoop::current()->task_runner())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);

  DCHECK(base::MessageLoop::current()->task_runner());
}

ScopedBaseEnvironment::~ScopedBaseEnvironment()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  {
    DCHECK(base::trace_event::MemoryDumpManager::GetInstance());
    base::trace_event::MemoryDumpManager::GetInstance()
      ->TeardownForTracing();
  }

  // save tracing report to file, if needed
  {
    const bool need_write_tracing_report
      = base::trace_event::TraceLog::GetInstance()->IsEnabled();
    if(need_write_tracing_report) {
      DCHECK(traceReportPath_);
      basis::writeTraceReport(
        *traceReportPath_);
    } else {
      LOG(INFO)
        << "tracing disabled";
    }
  }

  {
    LOG(INFO)
      << "ThreadPool Shutdown...";
    TRACE_EVENT0("shutdown", "MainLoop:ThreadPool");
    DCHECK(base::ThreadPool::GetInstance());
    base::ThreadPool::GetInstance()->Shutdown();
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
  , const base::FilePath& outDir
  , const base::FilePath::CharType icuFileName[]
  , const base::FilePath::CharType traceReportFileName[]
  , const int threadsNum)
{
  DCHECK(argc > 0);

  if (!base::PathService::Get(base::DIR_EXE, &dir_exe_)) {
    NOTREACHED();
    // stop app execution with EXIT_FAILURE
    return
      false;
  }

  traceReportPath_
    = std::make_unique<const base::FilePath>(
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

  base::PlatformThread::SetName("Main");

  // see https://stackoverflow.com/a/18981514/10904212
  std::locale::global(std::locale::classic());

  basis::initCommandLine(argc, argv);

#if DCHECK_IS_ON()
  // Must be done before hooking any functions that make stack traces.
  base::debug::EnableInProcessStackDumping();
#endif // DCHECK_IS_ON()

  base::SamplingHeapProfiler::Get()->SetRecordThreadNames(true);

  /// \todo
  // init allocator https://github.com/aranajhonny/chromium/blob/caf5bcb822f79b8997720e589334266551a50a13/content/app/content_main_runner.cc#L512

  // Enables 'terminate on heap corruption' flag. Helps protect against heap
  // overflow. Has no effect if the OS doesn't provide the necessary facility.
  /// \note On Linux, there nothing to do AFAIK.
  base::EnableTerminationOnHeapCorruption();

#if DCHECK_IS_ON()
  // Turns on process termination if memory runs out.
  base::EnableTerminationOnOutOfMemory();
#endif // DCHECK_IS_ON()

  base::FeatureList::InitializeInstance(
    std::string(), std::string());

  basis::initLogging(
    "" // logFile
    );

  if(!base::PathExists(dir_exe_.Append(icuFileName))) {
    LOG(ERROR)
        << "unable to load icu i18n data file: "
        << dir_exe_.Append(icuFileName);
    // stop app execution with EXIT_FAILURE
    return
      false;
  }

  icu_util::initICUi18n(icuFileName);

  /// \note you must init ICU before i18n
  i18n = std::make_unique<i18n::I18n>(
    nullptr // locale
    );

  {
    const int num_cores
      = base::SysInfo::NumberOfProcessors();
    const int kBackgroundMaxThreads
    /// \note based on command-line paramater
      = 1 + threadsNum;
    const int maxByDemandWorkerThreadsInPool
    /// \note based on command-line paramater
      = 1 + threadsNum;
    const int kForegroundMaxThreads
      = std::max(
          kBackgroundMaxThreads + maxByDemandWorkerThreadsInPool
          , num_cores - 1);
    CHECK(kForegroundMaxThreads >= 1)
      << "Unable to register foreground threads."
      " Make sure you have at leat one cpu core";

    basis::initThreadPool(
      kBackgroundMaxThreads
      , kForegroundMaxThreads);
  }

  // register basis::ApplicationPathKeys
  basis::AddPathProvider();

  // see http://dev.chromium.org/developers/how-tos/trace-event-profiling-tool
  basis::initTracing(
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
  // Is called from ctor but for browser it seems that it is more useful to
  // start logging after statistics recorder, so we need to init log-on-shutdown
  // later.
  // Usage examples:
  // UMA_HISTOGRAM_BOOLEAN("App.BoolTest()", false);
  // UMA_HISTOGRAM_COUNTS_100("App.TestCounts", 11);
  // UMA_HISTOGRAM_LONG_TIMES("App.TimeNow()", base::TimeDelta::FromMinutes(5));
  base::StatisticsRecorder::InitLogOnShutdown();

  // set current path
  {
    CHECK(!outDir.empty());
    base::SetCurrentDirectory(outDir);
    base::FilePath current_path;
    const bool curDirOk =
      base::GetCurrentDirectory(&current_path);
    DCHECK(curDirOk);
    VLOG(9)
        << "Current path is "
        << current_path;
  }

  return
    true;
}

} // basis
