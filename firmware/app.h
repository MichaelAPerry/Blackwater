/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "plat.h"
#include <stdint.h>
#define APP_NAME "BLACKWATER"
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u * 48u)
/* objects keep their numbers across releases: 0 and 1 as in the first build */
enum
{
    OBJ_SESSION, /* slot 1 */
    OBJ_PANEL,
    OBJ_SLOT2,
    OBJ_SLOT3,
    OBJ_SLOT4,
    OBJ_PATCHES,
    OBJ_LOG,
    OBJ_PREFS,
    OBJ_NOBJ
};
#define APP_STORE_MAP {0xA0000u, 0xDC000u, 0xA2000u, 0xA4000u, 0xA6000u, 0xA8000u, 0xAA000u, 0xAC000u}
#define APP_BACKUP_OBJS {OBJ_SESSION, OBJ_SLOT2, OBJ_SLOT3, OBJ_SLOT4, OBJ_PATCHES, OBJ_LOG, OBJ_PREFS}
#define APP_BACKUP_MAGIC 0x52544B42u /* "BKTR" */
#define APP_BACKUP_VERSION 1u
#define APP_BACKUP_WHAT "replaces the four sessions, your patches and the catch log"
int app_backup_playing(void);
void app_backup_flush(void);
void app_backup_hold(int);
void app_backup_restored(void);
#define APP_SPLASH
void app_splash(void);
void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
