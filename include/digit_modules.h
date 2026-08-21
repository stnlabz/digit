#ifndef STN_LABZ_DIGIT_MODULES_H
#define STN_LABZ_DIGIT_MODULES_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>


int digit_modules_init(void);

int digit_modules_is_ready(void);

FARPROC digit_modules_get_export(
    const char *module_id,
    const char *export_name
);

void digit_modules_shutdown(void);

#endif
