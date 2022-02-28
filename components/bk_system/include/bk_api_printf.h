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

void bk_printf(const char *fmt, ...);
void bk_null_printf(const char *fmt, ...);
int bk_printf_init(void);
int bk_printf_deinit(void);
void bk_set_printf_enable(uint8_t enable);
void bk_set_printf_sync(uint8_t enable);
int bk_get_printf_sync(void);
void bk_buf_printf_sync(char *buf, int buf_len);
void bk_printf_ex(int level, char * tag, const char *fmt, ...);
void bk_disable_mod_printf(char *mod_name, uint8_t disable);
char * bk_get_disable_mod(int * idx);

#ifdef __cplusplus
}
#endif
