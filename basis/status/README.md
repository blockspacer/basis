# About

Developer guide for error handling

## MUST SEE

Read comments in `status_macros.h`

## How to map custom error code to status code

Option 1: See https://github.com/stratum/stratum/blob/9f5bd2b285badbef11e81eca6c31d4a3c4342843/stratum/hal/lib/bcm/macros.h

Option 2: See `app_error_space.hpp`

## MUST READ

- http://www.furidamu.org/blog/2017/01/28/error-handling-with-statusor/
- https://github.com/sabbakumov/rst/blob/83004dfb406ffc5d3a791e71815320488b0d7b3d/README.md#statusor
- https://github.com/samiurkh1n/condit

## constructors

For constructors, the style guide is pretty specific not to do heavy work that could fail in constructors, and prefer factory functions (plus making the constructor private) to make sure you don?t get a half-formed object.

## Use `Status(Or)` for error handling

Explicitly declare your function to be capable of returning an error.

```cpp
Functions which can produce an error should return a tensorflow::Status. To propagate an error status, use the TF_RETURN_IF_ERROR macro.

TF_RETURN_IF_ERROR(f());

StatusOr
StatusOr<T> is the union of a Status object and a T object. It offers a way to use return values instead of output parameters for functions which may fail.

For example, consider the code:

Output out;
Status s = foo(&out);
if (!s.ok()) {
  return s;
}
out.DoSomething();
With StatusOr<T>, we can write this as

StatusOr<Output> result = foo();
if (!result.ok()) {
  return result.status();
}
result->DoSomething();
Pros:

Return values are easier to reason about than output parameters.

The types returned through StatusOr<T> don't need to support empty states. To return a type as an output parameter, we must either use a unique_ptr<T> or support an empty state for the type so that we can initialize the type before passing it as an output parameter. StatusOr<T> reduces the number of objects we have in an "uninitialized" state.

Cons:

StatusOr<T> adds complexity. It raises questions about what happens when T is null and how StatusOr<T> behaves during moves and copies. StatusOr<T> also generally comes with macros such as ASSIGN_OR_RETURN, which add additional complexity.

The current Tensorflow codebase exclusively uses Status instead of StatusOr<T>, so switching over would require a significant amount of work.

Decision:

Tensorflow foregoes the use of StatusOr<> because it doesn't add enough value to justify additional complexity.
```

```cpp
// StatusOr<T> is the union of a Status object and a T
// object. StatusOr models the concept of an object that is either a
// usable value, or an error Status explaining why such a value is
// not present. To this end, StatusOr<T> does not allow its Status
// value to be Status::OK. Furthermore, the value of a StatusOr<T*>
// must not be null. This is enforced by a debug check in most cases,
// but even when it is not, clients must not set the value to null.
//
// The primary use-case for StatusOr<T> is as the return value of a
// function which may fail.
//
// Example client usage for a StatusOr<T>, where T is not a pointer:
//
//  StatusOr<float> result = DoBigCalculationThatCouldFail();
//  if (result.ok()) {
//    float answer = result.ValueOrDie();
//    printf("Big calculation yielded: %f", answer);
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example client usage for a StatusOr<T*>:
//
//  StatusOr<Foo*> result = FooFactory::MakeNewFoo(arg);
//  if (result.ok()) {
//    std::unique_ptr<Foo> foo(result.ValueOrDie());
//    foo->DoSomethingCool();
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example client usage for a StatusOr<std::unique_ptr<T>>:
//
//  StatusOr<std::unique_ptr<Foo>> result = FooFactory::MakeNewFoo(arg);
//  if (result.ok()) {
//    std::unique_ptr<Foo> foo = std::move(result.ValueOrDie());
//    foo->DoSomethingCool();
//  } else {
//    LOG(ERROR) << result.status();
//  }
//
// Example factory implementation returning StatusOr<T*>:
//
//  StatusOr<Foo*> FooFactory::MakeNewFoo(int arg) {
//    if (arg <= 0) {
//      return tensorflow::InvalidArgument("Arg must be positive");
//    } else {
//      return new Foo(arg);
//    }
//  }
//
// Note that the assignment operators require that destroying the currently
// stored value cannot invalidate the argument; in other words, the argument
// cannot be an alias for the current value, or anything owned by the current
// value.
```

Example:

```cpp
::util::Status ReadProtoFromBinFile(const std::string& filename,
                                    ::google::protobuf::Message* message) {
  std::string buffer;
  RETURN_IF_ERROR(ReadFileToString(filename, &buffer));
  if (!message->ParseFromString(buffer)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Failed to parse the binary content of "
                                    << filename << " to proto.";
  }

  return ::util::OkStatus();
}

::util::StatusOr<std::unique_ptr<Query>> Adapter::Subscribe(
    const std::vector<Path>& paths,
    std::unique_ptr<ChannelWriter<PhalDB>> writer, absl::Duration poll_time) {
  ASSIGN_OR_RETURN(auto db_query, database_->MakeQuery(paths));
  RETURN_IF_ERROR(db_query->Subscribe(std::move(writer), poll_time));
  return db_query;
}

::util::Status BFSwitch::VerifyChassisConfig(const ChassisConfig& config) {
  // First make sure PHAL is happy with the config then continue with the rest
  // of the managers and nodes.
  absl::ReaderMutexLock l(&chassis_lock);
  ::util::Status status = ::util::OkStatus();
  APPEND_STATUS_IF_ERROR(status, phal_interface_->VerifyChassisConfig(config));
  APPEND_STATUS_IF_ERROR(status,
                         bf_chassis_manager_->VerifyChassisConfig(config));

  // Get the current copy of the node_id_to_unit from chassis manager. If this
  // fails with ERR_NOT_INITIALIZED, do not verify anything at the node level.
  // Note that we do not expect any change in node_id_to_unit. Any change in
  // this map will be detected in bcm_chassis_manager_->VerifyChassisConfig.
  auto ret = bf_chassis_manager_->GetNodeIdToUnitMap();
  if (!ret.ok()) {
    if (ret.status().error_code() != ERR_NOT_INITIALIZED) {
      APPEND_STATUS_IF_ERROR(status, ret.status());
    }
  } else {
    const auto& node_id_to_unit = ret.ValueOrDie();
    for (const auto& entry : node_id_to_unit) {
      uint64 node_id = entry.first;
      int unit = entry.second;
      ASSIGN_OR_RETURN(auto* pi_node, GetPINodeFromUnit(unit));
      APPEND_STATUS_IF_ERROR(status,
                             pi_node->VerifyChassisConfig(config, node_id));
    }
  }

  if (status.ok()) {
    LOG(INFO) << "Chassis config verified successfully.";
  }

  return status;
}

::util::Status BFChassisManager::UnregisterEventWriters() {
  absl::WriterMutexLock l(&chassis_lock);
  ::util::Status status = ::util::OkStatus();
  APPEND_STATUS_IF_ERROR(
      status, bf_pal_interface_->PortStatusChangeUnregisterEventWriter());
  if (!port_status_change_event_channel_->Close()) {
    APPEND_ERROR(status)
        << "Error when closing port status change event channel.";
  }
  if (xcvr_event_writer_id_ != kInvalidWriterId) {
    APPEND_STATUS_IF_ERROR(status,
                           phal_interface_->UnregisterTransceiverEventWriter(
                               xcvr_event_writer_id_));
    xcvr_event_writer_id_ = kInvalidWriterId;
    if (!xcvr_event_channel_->Close()) {
      APPEND_ERROR(status) << "Error when closing transceiver event channel.";
    }
  } else {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Transceiver event handler not registered.";
  }

  port_status_change_event_thread_.join();
  // Once the thread is joined, it is safe to reset these pointers.
  port_status_change_event_reader_ = nullptr;
  port_status_change_event_channel_ = nullptr;

  xcvr_event_thread_.join();
  xcvr_event_reader_ = nullptr;
  xcvr_event_channel_ = nullptr;
  return status;
}

::util::StatusOr<int> BFChassisManager::GetUnitFromNodeId(
    uint64 node_id) const {
  if (!initialized_) {
    return MAKE_ERROR(ERR_NOT_INITIALIZED) << "Not initialized!";
  }
  const int* unit = gtl::FindOrNull(node_id_to_unit_, node_id);
  CHECK_RETURN_IF_FALSE(unit != nullptr)
      << "Node " << node_id << " is not configured or not known.";

  return *unit;
}
```

Where `Code` is one of:

```cpp
enum Code {
  OK = 0,
  CANCELLED = 1,
  UNKNOWN = 2,
  INVALID_ARGUMENT = 3,
  DEADLINE_EXCEEDED = 4,
  NOT_FOUND = 5,
  ALREADY_EXISTS = 6,
  PERMISSION_DENIED = 7,
  UNAUTHENTICATED = 16,
  RESOURCE_EXHAUSTED = 8,
  FAILED_PRECONDITION = 9,
  ABORTED = 10,
  OUT_OF_RANGE = 11,
  UNIMPLEMENTED = 12,
  INTERNAL = 13,
  UNAVAILABLE = 14,
  DATA_LOSS = 15,
  DO_NOT_USE_RESERVED_FOR_FUTURE_EXPANSION_USE_DEFAULT_IN_SWITCH_INSTEAD_ = 20,
  // **DO NOT ADD ANYTHING TO THIS**
};
```
