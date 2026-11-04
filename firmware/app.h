/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "plat.h"
#include <stdint.h>
#define APP_NAME "BLACKWATER"
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u * 48u)
enum
{
    OBJ_SESSION,
    OBJ_PANEL,
    OBJ_NOBJ
};
#define APP_STORE_MAP {0xA0000u, 0xDC000u}
void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
