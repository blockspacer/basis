#include "tests_common.h"

#if !defined(DOCTEST_CONFIG_DISABLE)
#include <basis/doctest_util.hpp>
#endif // DOCTEST_CONFIG_DISABLE

#include "basis/log/logger.hpp"
#include "basis/log/log_util.hpp"

#include <base/logging.h>
#include <base/i18n/icu_util.h>
#include <base/command_line.h>
#include <base/threading/platform_thread.h>
#include <base/base_switches.h>
#include <base/feature_list.h>
#include <base/at_exit.h>
#include <base/message_loop/message_loop.h>

#include <base/bind.h>
#include <base/test/launcher/unit_test_launcher.h>
#include <base/test/test_suite.h>
#include <build/build_config.h>

#include <basis/numerics/uint128.hpp>

static inline void initI18n()
{
  /// \todo InitializeICUWithFileDescriptor
  bool icu_initialized = ::base::i18n::InitializeICU();
  //DCHECK(icu_initialized);
}

static inline void initCommandLine(int argc, char* argv[])
{
  ::base::PlatformThread::SetName("Main");

  // see https://stackoverflow.com/a/18981514/10904212
  std::locale::global(std::locale::classic());

  // see https://peter.sh/experiments/chromium-command-line-switches/
  ::base::CommandLine::Init(argc, argv);

  ::base::CommandLine* command_line = ::base::CommandLine::ForCurrentProcess();

  // initialize |g_vlog_info| in debug mode
#if !defined(NDEBUG)
  // Gives the default maximal active V-logging level; 0 is the default.
  // Normally positive values are used for V-logging levels.
  if(!command_line->HasSwitch(switches::kV)) {
    command_line->AppendSwitchASCII(switches::kV,
      "1");
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
  }

  // Indicates that crash reporting should be enabled. On platforms where helper
  // processes cannot access to files needed to make this decision, this flag is
  // generated internally.
  if(!command_line->HasSwitch(switches::kEnableCrashReporter)) {
    command_line->AppendSwitchASCII(switches::kEnableCrashReporter,
      "1");
  }

  // Generates full memory crash dump.
  if(!command_line->HasSwitch(switches::kFullMemoryCrashReport)) {
    command_line->AppendSwitchASCII(switches::kFullMemoryCrashReport,
      "1");
  }

#if defined(OS_LINUX)
    // Controls whether or not retired instruction counts are surfaced for threads
    // in trace events on Linux.
    //
    // This flag requires the BPF sandbox to be disabled.  if(!command_line->HasSwitch(switches::kVModule)) {
    if(!command_line->HasSwitch(switches::kEnableThreadInstructionCount)) {
      command_line->AppendSwitchASCII(switches::kEnableThreadInstructionCount,
        "1");
    }
#endif

  DCHECK(command_line->HasSwitch(switches::kV) ||
      command_line->HasSwitch(switches::kVModule));
#endif // NDEBUG

  /// \todo
  // init allocator https://github.com/aranajhonny/chromium/blob/caf5bcb822f79b8997720e589334266551a50a13/content/app/content_main_runner.cc#L512
  // ::base::EnableTerminationOnHeapCorruption();
  // ::base::EnableTerminationOnOutOfMemory();
  // mojo::embedder::Init();
  // mojo::ServiceManager::GetInstance();
//#if !defined(OFFICIAL_BUILD)
//  ::base::debug::EnableInProcessStackDumping();
//#if defined(OS_WIN)
//  ::base::RouteStdioToConsole(false);
//#endif
//#endif

  ::base::FeatureList::InitializeInstance(std::string(), std::string());

  /// \todo
  //base::FeatureList::InitializeInstance(
  //    command_line->GetSwitchValueASCII(switches::kEnableFeatures),
  //    command_line->GetSwitchValueASCII(switches::kDisableFeatures));

  // DCHECK(!base::TaskScheduler::GetInstance());
  // // A one-per-process task scheduler is needed for usage of APIs in
  // // base/post_task.h
  // ::base::TaskScheduler::CreateAndStartWithDefaultParams("MainThreadPool");
  // DCHECK(base::TaskScheduler::GetInstance());
}

int main(int argc, char* argv[])
{
  initCommandLine(argc, argv);

  // This object instance is required (for example,
  // LazyInstance, MessageLoop).
  ::base::AtExitManager at_exit;

  /// \note creates ::base::MessageLoop::current()
  ::base::MessageLoopForIO main_thread_message_loop;

  initI18n();

  ::basis::initLogging(
    "" // logFile
  );

  // If the LogWorker is initialized then at scope exit the g3::shutDownLogging() will be called.
  // This is important since it protects from LOG calls from static or other entities that will go
  // out of scope at a later time.
  //
  // It can also be called manually:
  at_exit.RegisterTask(base::BindOnce(
    []
    ()
    {
      LOG(INFO) << "shutdown...";
    }
  ));

#if !defined(DOCTEST_CONFIG_DISABLE)
  // run test cases unless with --no-run
  doctest::Context doctestContext;
  basis::initDoctestOptions(std::ref(doctestContext));
  // apply command line - argc / argv
  doctestContext.applyCommandLine(argc, argv);
  const auto doctestResult = doctestContext.run();
  // query flags (and --exit) rely on this
  // propagate the result of the tests
  // query flags (and --exit) rely on this
  if(doctestContext.shouldExit()) {
    LOG(INFO) << "got shouldExit for doctest tests...";
    return doctestResult;
  } else {
    LOG(INFO) << "done doctest tests...";
  }
#endif // DOCTEST_CONFIG_DISABLE

  // start working on other parts of your project here.
  return 0;
}

#if !defined(DOCTEST_CONFIG_DISABLE)
DOCTEST_TEST_CASE("dummy") { printf("dummy doctest test\n"); }
#endif // DOCTEST_CONFIG_DISABLE
