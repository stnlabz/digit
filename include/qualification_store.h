#ifndef DIGIT_QUALIFICATION_STORE_H
#define DIGIT_QUALIFICATION_STORE_H

#include "qualification.h"

#define DIGIT_QUALIFICATION_STORE_PATH_MAX 1024

int digit_qualification_store_load(
    const char *path,
    digit_qualification_inventory_t *inventory
);

int digit_qualification_store_save(
    const char *path,
    const digit_qualification_inventory_t *inventory
);

#endif
