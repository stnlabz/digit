#ifndef STN_LABZ_DIGIT_MODULES_H
#define STN_LABZ_DIGIT_MODULES_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>


int digit_modules_init(void);

int digit_modules_is_ready(void);


/*
 * Acquire an exported module function for use.
 *
 * A successful acquisition prevents the module from
 * being unloaded until digit_modules_release_export()
 * is called.
 */
int digit_modules_acquire_export(
    const char *module_id,
    const char *export_name,
    FARPROC *export_out
);


/*
 * Release a previously acquired module export.
 */
void digit_modules_release_export(
    const char *module_id
);


void digit_modules_shutdown(void);

#endif