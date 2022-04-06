// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <os/os.h>

void rtos_dump_task_list(void);
void rtos_dump_stack_memory_usage(void);
void rtos_dump_task_runtime_stats(void);
void rtos_dump_task_backtrace(beken_thread_t *thread);
void rtos_dump_backtrace(void);
void rtos_assert_error(const char *condition,const char *func,int line);

#ifdef __cplusplus
}
#endif