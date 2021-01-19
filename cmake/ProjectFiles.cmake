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
  ${BASIS_DIR}/doctest_util.hpp
  ${BASIS_DIR}/doctest_util.cc
  #
  ${BASIS_DIR}/i18n/i18n.hpp
  ${BASIS_DIR}/i18n/i18n.cc
  #
  ${BASIS_DIR}/i18n/icu_util.hpp
  ${BASIS_DIR}/i18n/icu_util.cc
  #
  ${BASIS_DIR}/memory/any_ptr.hpp
  #
  ${BASIS_DIR}/cmd_util.hpp
  ${BASIS_DIR}/cmd_util.cc
  #
  ${BASIS_DIR}/threading/thread_pool_util.hpp
  ${BASIS_DIR}/threading/thread_pool_util.cc
  ${BASIS_DIR}/threading/thread_health_checker.hpp
  ${BASIS_DIR}/threading/thread_health_checker.cc
  #
  ${BASIS_DIR}/varint/varint.hpp
  ${BASIS_DIR}/varint/varint.cc
  ${BASIS_DIR}/varint/fast_varint.hpp
  ${BASIS_DIR}/varint/fast_varint.cc
  ${BASIS_DIR}/varint/memcmpable_varint.hpp
  ${BASIS_DIR}/varint/memcmpable_varint.cc
  #
  ${BASIS_DIR}/bloom_filter/filter_policy.hpp
  ${BASIS_DIR}/bloom_filter/filter_policy.cc
  ${BASIS_DIR}/bloom_filter/bloom.hpp
  ${BASIS_DIR}/bloom_filter/bloom.cc
  ${BASIS_DIR}/bloom_filter/double_hashing_bloom.hpp
  ${BASIS_DIR}/bloom_filter/double_hashing_bloom.cc
  #
  ${BASIS_DIR}/tracing/tracing_util.hpp
  ${BASIS_DIR}/tracing/tracing_util.cc
  ${BASIS_DIR}/tracing/local_trace.hpp
  ${BASIS_DIR}/tracing/stats_table.hpp
  ${BASIS_DIR}/tracing/stats_table.cc
  #
  ${BASIS_DIR}/jsonb/jsonb.cc
  ${BASIS_DIR}/jsonb/jsonb.hpp
  #
  ${BASIS_DIR}/metrics/metrics.cc
  ${BASIS_DIR}/metrics/metrics.hpp
  #
  ${BASIS_DIR}/strong_types/strong_alias.hpp
  ${BASIS_DIR}/strong_types/strong_int.hpp
  ${BASIS_DIR}/strong_types/strong_string.hpp
  ${BASIS_DIR}/strong_types/strong_bool.hpp
  ${BASIS_DIR}/strong_types/metric_types/strong_metric_units.hpp
  ${BASIS_DIR}/strong_types/size_types/strong_size_units.hpp
  ${BASIS_DIR}/strong_types/time_types/strong_time_units.hpp
  ${BASIS_DIR}/strong_types/money/money.hpp
  ${BASIS_DIR}/strong_types/money/money_util.hpp
  ${BASIS_DIR}/strong_types/money/money_util.cc
  #
  ${BASIS_DIR}/core/immutable.hpp
  ${BASIS_DIR}/core/numeric_static_cast.hpp
  ${BASIS_DIR}/core/pimpl.hpp
  ${BASIS_DIR}/core/weak_handle.hpp
  ${BASIS_DIR}/core/enum_set.hpp
  ${BASIS_DIR}/core/bitmask.hpp
  ${BASIS_DIR}/core/typed_enum.hpp
  ${BASIS_DIR}/core/polymorphic_downcast.hpp
  ${BASIS_DIR}/core/polymorphic_upcast.hpp
  ${BASIS_DIR}/core/sequence_bound_flag.cc
  ${BASIS_DIR}/core/sequence_bound_flag.hpp
  ${BASIS_DIR}/core/retrier.hpp
  ${BASIS_DIR}/core/weak_handle.cc
  ${BASIS_DIR}/core/weak_handle.hpp
  #
  ${BASIS_DIR}/task/prioritized_once_task_heap.hpp
  ${BASIS_DIR}/task/prioritized_once_task_heap.cc
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
  ${BASIS_DIR}/status/app_error_space.hpp
  ${BASIS_DIR}/status/app_error_space.cc
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
  ${BASIS_DIR}/promise/do_nothing_promise.h
  ${BASIS_DIR}/promise/do_nothing_promise.cc
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
  ${BASIS_DIR}/container/prioritized_list.hpp
  ${BASIS_DIR}/container/seq_num_buffer.hpp
  ${BASIS_DIR}/container/prioritized_job_dispatcher.hpp
  ${BASIS_DIR}/container/prioritized_job_dispatcher.cc
  ${BASIS_DIR}/container/prioritized_repeating_task_list.hpp
  ${BASIS_DIR}/container/prioritized_repeating_task_list.cc
  #
  ${BASIS_DIR}/time_step/fixed_time_step.hpp
  ${BASIS_DIR}/time_step/fixed_time_step.cc
  ${BASIS_DIR}/time_step/fixed_time_step_loop.hpp
  ${BASIS_DIR}/time_step/fixed_time_step_loop.cc
  #
  ${BASIS_DIR}/numerics/uint128.hpp
  ${BASIS_DIR}/numerics/math_utils.hpp
  #
  ${BASIS_DIR}/concurrency/lock_free_producer_consumer_queue.hpp
  ${BASIS_DIR}/concurrency/lock_free_producer_consumer_queue.cc
  ${BASIS_DIR}/concurrency/concurrent_int64.hpp
  ${BASIS_DIR}/concurrency/concurrent_int64.cc
  #
  ${BASIS_DIR}/log/logger.cc
  ${BASIS_DIR}/log/logger.hpp
  ${BASIS_DIR}/log/log_util.hpp
  ${BASIS_DIR}/log/log_util.cc
  ${BASIS_DIR}/log/progress_logger.cc
  ${BASIS_DIR}/log/progress_logger.hpp
  ${BASIS_DIR}/log/spacer.cc
  ${BASIS_DIR}/log/spacer.hpp
  ${BASIS_DIR}/log/scoped_log_run_time.hpp
  ${BASIS_DIR}/log/scoped_log_run_time.cc
  ${BASIS_DIR}/log/table_printer.hpp
  ${BASIS_DIR}/log/table_printer.cc
  #
  ${BASIS_DIR}/task/task_util.cc
  ${BASIS_DIR}/task/task_util.hpp
  ${BASIS_DIR}/task/periodic_check.cc
  ${BASIS_DIR}/task/periodic_check.hpp
  ${BASIS_DIR}/task/periodic_validate_until.cc
  ${BASIS_DIR}/task/periodic_validate_until.hpp
  ${BASIS_DIR}/task/alarm_manager.hpp
  ${BASIS_DIR}/task/alarm_manager.cc
  #
  ${BASIS_DIR}/state_machine/unsafe_state_machine.hpp
  #
  ${BASIS_DIR}/backoff_entry/backoff_entry_serializer.hpp
  ${BASIS_DIR}/backoff_entry/backoff_entry_serializer.cc
  ${BASIS_DIR}/backoff_entry/backoff_entry.hpp
  ${BASIS_DIR}/backoff_entry/backoff_entry.cc
  #
  ${BASIS_DIR}/checks_and_guard_annotations.hpp
  ${BASIS_DIR}/checks_and_guard_annotations.cc
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
  ${BASIS_DIR}/fail_point/fail_point.hpp
  ${BASIS_DIR}/fail_point/fail_point.cc
  #
  ${BASIS_DIR}/plug_point/plug_point.hpp
  ${BASIS_DIR}/plug_point/plug_point.cc
  #
  ${BASIS_DIR}/ECS/components/relationship/parent_entity.hpp
  #
  ${BASIS_DIR}/ECS/components/relationship/first_child_in_linked_list.hpp
  #
  ${BASIS_DIR}/ECS/components/relationship/top_level_children_count.hpp
  #
  ${BASIS_DIR}/ECS/components/relationship/child_siblings.hpp
  #
  ${BASIS_DIR}/ECS/helpers/relationship/remove_child_from_top_level.hpp
  #
  ${BASIS_DIR}/ECS/helpers/relationship/foreach_top_level_child.hpp
  #
  ${BASIS_DIR}/ECS/helpers/relationship/view_top_level_children.hpp
  #
  ${BASIS_DIR}/ECS/helpers/relationship/prepend_child_entity.hpp
  #
  ${BASIS_DIR}/ECS/helpers/lifetime/populate_delayed_construction_components.hpp
  ${BASIS_DIR}/ECS/helpers/lifetime/populate_delayed_construction_components.cc
  #
  ${BASIS_DIR}/bind/verify_nothing.hpp
  ${BASIS_DIR}/bind/exec_time_checker.hpp
  ${BASIS_DIR}/bind/exec_time_checker.cc
  ${BASIS_DIR}/bind/callable_hook.hpp
  ${BASIS_DIR}/bind/callable_hook.cc
  ${BASIS_DIR}/bind/delay_time_checker.hpp
  ${BASIS_DIR}/bind/delay_time_checker.cc
  #
  ${BASIS_DIR}/serialization/serializers.cc
  ${BASIS_DIR}/serialization/serializers.hpp
  #
  ${BASIS_DIR}/static_sequence/static_sequence.cc
  ${BASIS_DIR}/static_sequence/static_sequence.hpp
  #
  ${BASIS_DIR}/strings/string_util.cc
  ${BASIS_DIR}/strings/string_util.hpp
  #
  ${BASIS_DIR}/time/system_time_change_notifier.cc
  ${BASIS_DIR}/time/system_time_change_notifier.hpp
  #
  ${BASIS_DIR}/time/interval_timer.hpp
  ${BASIS_DIR}/time/interval_timer.cc
  ${BASIS_DIR}/time/backoff_timer.hpp
  ${BASIS_DIR}/time/backoff_timer.cc
  #
  ${BASIS_DIR}/files/file_path_util.cc
  ${BASIS_DIR}/files/file_path_util.hpp
  ${BASIS_DIR}/files/scoped_temp_file.cc
  ${BASIS_DIR}/files/scoped_temp_file.hpp
  #
  ${BASIS_DIR}/interval/interval.hpp
  ${BASIS_DIR}/interval/interval_set.hpp
  #
  ${BASIS_DIR}/statistics/usage_limiter.cc
  ${BASIS_DIR}/statistics/usage_limiter.hpp
  ${BASIS_DIR}/statistics/rate_tracker.cc
  ${BASIS_DIR}/statistics/rate_tracker.hpp
  ${BASIS_DIR}/statistics/rate_tracker.cc
  ${BASIS_DIR}/statistics/expandable_statistics_window.hpp
  ${BASIS_DIR}/statistics/moving_statistics_window.hpp
  ${BASIS_DIR}/statistics/moving_average.cc
  ${BASIS_DIR}/statistics/moving_average.hpp
  ${BASIS_DIR}/statistics/quality_threshold.cc
  ${BASIS_DIR}/statistics/quality_threshold.hpp
  ${BASIS_DIR}/statistics/weighted_mean.hpp
  ${BASIS_DIR}/statistics/weighted_mean.cc
  ${BASIS_DIR}/statistics/weighted_moving_average.hpp
  ${BASIS_DIR}/statistics/weighted_moving_average.cc
  ${BASIS_DIR}/statistics/weighted_moving_linear_regression.hpp
  ${BASIS_DIR}/statistics/weighted_moving_linear_regression.cc
  ${BASIS_DIR}/statistics/hdr_histogram.hpp
  ${BASIS_DIR}/statistics/hdr_histogram.cc
  #
  ${BASIS_DIR}/dependency_hierarchy/dependency_hierarchy.cc
  ${BASIS_DIR}/dependency_hierarchy/dependency_hierarchy.hpp
  ${BASIS_DIR}/dependency_hierarchy/dependency_error_space.cc
  ${BASIS_DIR}/dependency_hierarchy/dependency_error_space.hpp
  ${BASIS_DIR}/dependency_hierarchy/dependency_util.cc
  ${BASIS_DIR}/dependency_hierarchy/dependency_util.hpp
  #
  ${BASIS_DIR}/ECS/unsafe_context.cc
  ${BASIS_DIR}/ECS/unsafe_context.hpp
  #
  ${BASIS_DIR}/ECS/sequence_local_context.cc
  ${BASIS_DIR}/ECS/sequence_local_context.hpp
  #
  ${BASIS_DIR}/ECS/safe_registry.cc
  ${BASIS_DIR}/ECS/safe_registry.hpp
  #
  ${BASIS_DIR}/ECS/ecs.hpp
  ${BASIS_DIR}/ECS/ecs.cc
  #
  ${BASIS_DIR}/ECS/tags.hpp
  ${BASIS_DIR}/ECS/tags.cc
)
