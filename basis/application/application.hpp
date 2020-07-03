#pragma once

#include "basis/application/paths/path_provider.hpp"
#include "basis/application/paths/application_paths.hpp"
#include "basis/application/application_state.hpp"

#include <base/macros.h>
#include <base/logging.h>
#include <base/sequenced_task_runner.h>
#include <base/observer_list_threadsafe.h>
#include <base/synchronization/waitable_event.h>

namespace application {

class ApplicationObserver {
 public:
  ApplicationObserver();

  virtual
    ~ApplicationObserver();

  virtual
    void
      onStateChange(
        const application::ApplicationState state)
          = 0;

  virtual
    void
      onFocusChange(
        const bool has_focus)
          = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ApplicationObserver);
};

class Application {
 public:
  Application();

  ~Application();

  // Add a non owning pointer
  void
    addObserver(
      ApplicationObserver* observer);

  // Does nothing if the |observer| is
  // not in the list of known observers.
  void
    removeObserver(
      ApplicationObserver* observer);

  // Notify |Observer|s
  void
    notifyStateChange(
      const application::ApplicationState& state);

  // Notify |Observer|s
  void
    notifyFocusChange(
      const bool has_focus);

  void
    initialize();

  void
    teardown();

  void
    suspend();

  void
    pause();

  void
    resume();

  void
    start();

  /// \todo remove and use Promise::All()
  // wait for `application.signalOnLoad()`
  bool
    waitForLoad(
      const base::TimeDelta& timeout); /// \todo remove

  /// \todo remove and use Promise::All()
  void
    signalOnLoad(); /// \todo remove

  application::ApplicationState
    getApplicationState()
    const
    noexcept;

  void
    setApplicationState(
      application::ApplicationState state);

  bool
    HasFocus(application::ApplicationState state);

private:
  SEQUENCE_CHECKER(sequence_checker_);

  // The current application state.
  application::ApplicationState application_state_
    = application::kApplicationStatePreloading;

  friend class ApplicationObserver;

  /// \note ObserverListThreadSafe may be ued from multiple threads
  const scoped_refptr<
      base::ObserverListThreadSafe<ApplicationObserver>
    > observers_;

  base::WaitableEvent app_loaded_; /// \todo remove

  DISALLOW_COPY_AND_ASSIGN(Application);
};

} // namespace application
