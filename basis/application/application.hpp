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

struct ApplicationStateTransition
{
  application::ApplicationState new_state;

  application::ApplicationState prev_state;
};

class ApplicationStateObserver {
 public:
  ApplicationStateObserver();

  virtual
    ~ApplicationStateObserver();

  virtual
    void
      onStateChange(
        const application::ApplicationStateTransition stateTransition)
          = 0;

  virtual
    void
      onFocusChange(
        const bool has_focus)
          = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ApplicationStateObserver);
};

class ApplicationStateManager {
 public:
  ApplicationStateManager();

  ~ApplicationStateManager();

  // Add a non owning pointer
  void
    addObserver(
      ApplicationStateObserver* observer);

  // Does nothing if the |observer| is
  // not in the list of known observers.
  void
    removeObserver(
      ApplicationStateObserver* observer);

  // Notify |Observer|s
  void
    notifyStateChange(
      const application::ApplicationState& new_state);

  // Notify |Observer|s
  void
    notifyFocusChange(
      const bool has_focus);

  void
    initialize();

  // |stop| is teardown
  // i.e. not same as |pause|
  void
    stop();

  void
    suspend();

  void
    pause();

  void
    resume();

  void
    start();

  application::ApplicationState
    getApplicationState()
    const
    noexcept;

  void
    setApplicationState(
      application::ApplicationState state);

  // only |application::kApplicationStateStarted| has focus
  bool
    HasFocus(application::ApplicationState state);

private:
  SEQUENCE_CHECKER(sequence_checker_);

  // The current application state.
  application::ApplicationState application_state_
    = application::kApplicationStatePreloading;

  friend class ApplicationStateObserver;

  /// \note ObserverListThreadSafe may be ued from multiple threads
  const scoped_refptr<
      base::ObserverListThreadSafe<ApplicationStateObserver>
    > observers_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationStateManager);
};

} // namespace application
