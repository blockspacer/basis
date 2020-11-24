﻿include_guard( DIRECTORY )

list(APPEND BASIS_SOURCES
  ${BASIS_DIR}/application/app_runners.h
  ${BASIS_DIR}/application/app_runners.cc
  #
  ${BASIS_DIR}/base_environment.hpp
  ${BASIS_DIR}/base_environment.cc
  #
  ${BASIS_DIR}/path_provider.hpp
  ${BASIS_DIR}/path_provider.cc
  #
  ${BASIS_DIR}/cmd_util.hpp
  ${BASIS_DIR}/cmd_util.cc
  #
  ${BASIS_DIR}/PluginManager.hpp
  ${BASIS_DIR}/PluginManager.cc
  #
  ${BASIS_DIR}/boost_command_line.hpp
  ${BASIS_DIR}/boost_command_line.cc
  #
  ${BASIS_DIR}/doctest_util.hpp
  ${BASIS_DIR}/doctest_util.cc
  #
  ${BASIS_DIR}/i18n/i18n.hpp
  ${BASIS_DIR}/i18n/i18n.cc
  #
  ${BASIS_DIR}/i18n/icu_util.hpp
  ${BASIS_DIR}/i18n/icu_util.cc
  #
  ${BASIS_DIR}/cmd_util.hpp
  ${BASIS_DIR}/cmd_util.cc
  #
  ${BASIS_DIR}/threading/thread_pool_util.hpp
  ${BASIS_DIR}/threading/thread_pool_util.cc
  #
  ${BASIS_DIR}/tracing/tracing_util.hpp
  ${BASIS_DIR}/tracing/tracing_util.cc
  #
  ${BASIS_DIR}/tracing/local_trace.hpp
  #
  ${BASIS_DIR}/tracing/stats_table.hpp
  ${BASIS_DIR}/tracing/stats_table.cc
  #
  ${BASIS_DIR}/scoped_log_run_time.hpp
  ${BASIS_DIR}/scoped_log_run_time.cc
  #
  ${BASIS_DIR}/strong_types/strong_alias.hpp
  #
  ${BASIS_DIR}/core/immutable.hpp
  ${BASIS_DIR}/core/numeric_static_cast.hpp
  ${BASIS_DIR}/core/pimpl.hpp
  ${BASIS_DIR}/core/weak_handle.hpp
  #
  ${BASIS_DIR}/enum_set.hpp
  ${BASIS_DIR}/polymorphic_downcast.hpp
  #
  ${BASIS_DIR}/task/periodic_prioritized_task_heap.hpp
  ${BASIS_DIR}/task/periodic_prioritized_task_heap.cc
  #
  ${BASIS_DIR}/task/periodic_task_executor.hpp
  ${BASIS_DIR}/task/periodic_task_executor.cc
  #
  ${BASIS_DIR}/status/status.hpp
  ${BASIS_DIR}/status/status.cc
  ${BASIS_DIR}/status/statusor.hpp
  ${BASIS_DIR}/status/statusor.cc
  ${BASIS_DIR}/status/status_macros.hpp
  ${BASIS_DIR}/status/status_macros.cc
  ${BASIS_DIR}/status/posix_error_space.hpp
  ${BASIS_DIR}/status/posix_error_space.cc
  #
  ${BASIS_DIR}/application/application.hpp
  ${BASIS_DIR}/application/application.cc
  #
  ${BASIS_DIR}/application/application_configuration.hpp
  ${BASIS_DIR}/application/application_state.hpp
  ${BASIS_DIR}/application/export.hpp
  ${BASIS_DIR}/application/platform_detection.hpp
  #
  ${BASIS_DIR}/application/paths/path_provider.hpp
  ${BASIS_DIR}/application/paths/application_paths.hpp
  ${BASIS_DIR}/application/paths/application_get_path.hpp
  ${BASIS_DIR}/application/paths/path_id.hpp
  ${BASIS_DIR}/application/paths/path_provider.cc
  #
  ${BASIS_DIR}/application/posix/paths/application_get_path.cc
  ${BASIS_DIR}/application/posix/application_configuration_posix.hpp
  #
  ${BASIS_DIR}/test/do_nothing_promise.h
  ${BASIS_DIR}/test/do_nothing_promise.cc
  #
  ${BASIS_DIR}/promise/abstract_promise.cc
  ${BASIS_DIR}/promise/abstract_promise.h
  ${BASIS_DIR}/promise/all_container_executor.h
  ${BASIS_DIR}/promise/all_tuple_executor.h
  ${BASIS_DIR}/promise/dependent_list.cc
  ${BASIS_DIR}/promise/dependent_list.h
  ${BASIS_DIR}/promise/finally_executor.cc
  ${BASIS_DIR}/promise/finally_executor.h
  ${BASIS_DIR}/promise/helpers.cc
  ${BASIS_DIR}/promise/helpers.h
  ${BASIS_DIR}/promise/no_op_promise_executor.cc
  ${BASIS_DIR}/promise/post_task_executor.h
  ${BASIS_DIR}/promise/promise.h
  ${BASIS_DIR}/promise/promise.h
  ${BASIS_DIR}/promise/race_container_executor.h
  ${BASIS_DIR}/promise/race_tuple_executor.h
  ${BASIS_DIR}/promise/promise_executor.cc
  ${BASIS_DIR}/promise/promise_executor.h
  ${BASIS_DIR}/promise/promise_result.h
  ${BASIS_DIR}/promise/promise_value.cc
  ${BASIS_DIR}/promise/promise_value.h
  ${BASIS_DIR}/promise/post_promise.cc
  ${BASIS_DIR}/promise/post_promise.h
  ${BASIS_DIR}/promise/then_and_catch_executor.cc
  ${BASIS_DIR}/promise/then_and_catch_executor.h
  #
  ${BASIS_DIR}/time_step/FixedTimeStep.hpp
  ${BASIS_DIR}/time_step/FixedTimeStep.cc
  ${BASIS_DIR}/time_step/FixedTimeStepLoop.hpp
  ${BASIS_DIR}/time_step/FixedTimeStepLoop.cc
  #
  ${BASIS_DIR}/timer/IntervalTimer.hpp
  ${BASIS_DIR}/timer/IntervalTimer.cc
  #
  ${BASIS_DIR}/concurrency/LockFreeProducerConsumerQueue.hpp
  ${BASIS_DIR}/concurrency/LockFreeProducerConsumerQueue.cc
  #
  ${BASIS_DIR}/log/logger.cc
  ${BASIS_DIR}/log/logger.hpp
  ${BASIS_DIR}/log/log_util.hpp
  ${BASIS_DIR}/log/log_util.cc
  #
  ${BASIS_DIR}/task/task_util.cc
  ${BASIS_DIR}/task/task_util.hpp
  #
  ${BASIS_DIR}/task/periodic_check.cc
  ${BASIS_DIR}/task/periodic_check.hpp
  #
  ${BASIS_DIR}/task/periodic_validate_until.cc
  ${BASIS_DIR}/task/periodic_validate_until.hpp
  #
  ${BASIS_DIR}/state_machine/unsafe_state_machine.hpp
  #
  ${BASIS_DIR}/scoped_sequence_context_var.hpp
  ${BASIS_DIR}/scoped_sequence_context_var.cc
  #
  ${BASIS_DIR}/scoped_checks.hpp
  ${BASIS_DIR}/scoped_checks.cc
  #
  ${BASIS_DIR}/verify_nothing.hpp
  #
  ${BASIS_DIR}/checked_optional.hpp
  #
  ${BASIS_DIR}/unowned_ptr.hpp
  #
  ${BASIS_DIR}/unowned_ptr.hpp
  #
  ${BASIS_DIR}/unowned_ref.hpp
  #
  ${BASIS_DIR}/promise/promise_collection.hpp
  #
  ${BASIS_DIR}/progress_logger.cc
  ${BASIS_DIR}/progress_logger.hpp
  #
  ${BASIS_DIR}/sequence_bound_flag.cc
  ${BASIS_DIR}/sequence_bound_flag.hpp
  #
  ${BASIS_DIR}/fail_point/fail_point.hpp
  ${BASIS_DIR}/fail_point/fail_point.cc
  #
  ${BASIS_DIR}/plug_point/plug_point.hpp
  ${BASIS_DIR}/plug_point/plug_point.cc
  #
  ${BASIS_DIR}/ECS/components/parent_entity.hpp
  #
  ${BASIS_DIR}/ECS/components/first_child_in_linked_list.hpp
  #
  ${BASIS_DIR}/ECS/components/child_linked_list_size.hpp
  #
  ${BASIS_DIR}/ECS/components/child_linked_list.hpp
  #
  ${BASIS_DIR}/ECS/helpers/remove_child_entity.hpp
  #
  ${BASIS_DIR}/ECS/helpers/foreach_child_entity.hpp
  #
  ${BASIS_DIR}/ECS/helpers/view_child_entities.hpp
  #
  ${BASIS_DIR}/ECS/helpers/prepend_child_entity.hpp
  #
  ${BASIS_DIR}/bind/exec_time_checker.hpp
  ${BASIS_DIR}/bind/exec_time_checker.cc
  ${BASIS_DIR}/bind/callable_hook.hpp
  ${BASIS_DIR}/bind/callable_hook.cc
  #
  ${BASIS_DIR}/ECS/simulation_registry.cc
  ${BASIS_DIR}/ECS/simulation_registry.hpp
  #
  ${BASIS_DIR}/ECS/global_context.cc
  ${BASIS_DIR}/ECS/global_context.hpp
  #
  ${BASIS_DIR}/ECS/unsafe_context.cc
  ${BASIS_DIR}/ECS/unsafe_context.hpp
  #
  ${BASIS_DIR}/ECS/sequence_local_context.cc
  ${BASIS_DIR}/ECS/sequence_local_context.hpp
  #
  ${BASIS_DIR}/ECS/network_registry.cc
  ${BASIS_DIR}/ECS/network_registry.hpp
  #
  ${BASIS_DIR}/ECS/ecs.hpp
  #
  ${BASIS_DIR}/ECS/tags.hpp
)
