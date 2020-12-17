// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "basis/task/alarm_manager.hpp" // IWYU pragma: associated

#include <utility>

#include <base/bind.h>
#include <base/logging.h>
#include <base/single_thread_task_runner.h>
#include <base/threading/thread_task_runner_handle.h>
#include <base/time/clock.h>
#include <base/time/default_clock.h>
#include <base/rvalue_cast.h>

#define MAKE_SURE_OWN_THREAD(callback, ...)                                    \
  if (!task_runner_->BelongsToCurrentThread()) {                               \
    task_runner_->PostTask(                                                    \
        FROM_HERE, base::BindOnce(&AlarmManager::callback,                     \
                                  weak_factory_.GetWeakPtr(), ##__VA_ARGS__)); \
    return;                                                                    \
  }

namespace basis {

namespace {

void VerifyHandleCallback(base::OnceClosure task,
                          base::WeakPtr<AlarmHandle> handle) {
  if (!handle.get()) {
    return;
  }
  ::base::rvalue_cast(task).Run();
}
}  // namespace

AlarmManager::AlarmInfo::AlarmInfo(
    base::OnceClosure task,
    base::Time time,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_(::base::rvalue_cast(task)),
      time_(time),
      task_runner_(::base::rvalue_cast(task_runner)) {}

AlarmManager::AlarmInfo::~AlarmInfo() {}

void AlarmManager::AlarmInfo::PostTask() {
  task_runner_->PostTask(FROM_HERE, ::base::rvalue_cast(task_));
}

AlarmManager::AlarmManager(
    std::unique_ptr<base::Clock> clock,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner
    , const base::TimeDelta& polling_frequency)
    : clock_(::base::rvalue_cast(clock)),
      task_runner_(::base::rvalue_cast(task_runner)),
      weak_factory_(this) {
  DCHECK(clock_);
  DCHECK(task_runner_);
  clock_tick_timer_.SetTaskRunner(task_runner_);
  clock_tick_timer_.Start(FROM_HERE, polling_frequency,
                          base::BindRepeating(&AlarmManager::CheckAlarm,
                                              weak_factory_.GetWeakPtr()));
}

AlarmManager::AlarmManager()
    : AlarmManager(std::make_unique<base::DefaultClock>(),
                   base::ThreadTaskRunnerHandle::Get()) {}

AlarmManager::~AlarmManager() {}

std::unique_ptr<AlarmHandle> AlarmManager::PostAlarmTask(base::OnceClosure task,
                                                         base::Time time) {
  DCHECK(task);
  std::unique_ptr<AlarmHandle> handle = std::make_unique<AlarmHandle>();
  AddAlarm(base::BindOnce(&VerifyHandleCallback, ::base::rvalue_cast(task),
                          handle->AsWeakPtr()),
           time, base::ThreadTaskRunnerHandle::Get());
  return handle;
}

void AlarmManager::AddAlarm(
    base::OnceClosure task,
    base::Time time,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  MAKE_SURE_OWN_THREAD(AddAlarm, ::base::rvalue_cast(task), time, ::base::rvalue_cast(task_runner));
  next_alarm_.push(std::make_unique<AlarmInfo>(::base::rvalue_cast(task), time,
                                               ::base::rvalue_cast(task_runner)));
}

void AlarmManager::CheckAlarm() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  base::Time now = clock_->Now();
  // Fire appropriate alarms.
  while (!next_alarm_.empty() && now >= next_alarm_.top()->time()) {
    next_alarm_.top()->PostTask();
    next_alarm_.pop();
  }
}

}  // namespace basis