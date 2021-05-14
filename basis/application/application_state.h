#pragma once

#include <base/logging.h>
#include <base/notreached.h>
#include <base/macros.h>

#include <basic/macros.h>

namespace application {

enum ApplicationState {
  // The application is expected to be able to move back into
  // the Started state very quickly
  kApplicationStatePaused,

  // A possible initial state where the application can be running,
  // loading data, and so on, but is not visible to the user.
  kApplicationStatePreloading,

  // The state where the application is running in the foreground,
  // fully visible, with all necessary resources available.
  // A possible initial state, where loading happens
  // while in the foreground.
  kApplicationStateStarted,

  // Representation of a idle/terminal/shutdown state
  // with no resources.
  kApplicationStateStopped,

  // The application was running at some point,
  // but has been backgrounded to the
  // point where resources are invalid
  // and execution should be halted
  // until resumption.
  kApplicationStateSuspended,

  kApplicationStateTotal
};

// Returns a human-readable string for the given |state|.
static inline const char *GetApplicationStateString(ApplicationState state)
{
  switch (state) {
    case kApplicationStatePaused:
      return "kApplicationStatePaused";
    case kApplicationStatePreloading:
      return "kApplicationStatePreloading";
    case kApplicationStateStarted:
      return "kApplicationStateStarted";
    case kApplicationStateStopped:
      return "kApplicationStateStopped";
    case kApplicationStateSuspended:
      return "kApplicationStateSuspended";
    case kApplicationStateTotal: {
      NOTREACHED() << "state = " << state;
      return "INVALID_APPLICATION_STATE";
    }
  }

  NOTREACHED() << "state = " << state;
  return "INVALID_APPLICATION_STATE";
}

} // namespace application
