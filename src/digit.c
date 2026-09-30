#include <stdio.h>

#include "digit.h"

int digit_initialize(void)
{
    printf("%s\n", DIGIT_NAME);
    printf("Digit is using the STN-LABZ ABI version %s\n", DIGIT_ABI_VERSION);
    printf("Core initialization: READY\n\n");
    printf("Greetings.\n\n");
    printf("What is today's mission?\n");

    return 0;
}
