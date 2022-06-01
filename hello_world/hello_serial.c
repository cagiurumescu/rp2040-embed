/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    int i=0;
    while (true) {
        printf("Hello, world %d!\n", i);
        i++;
        sleep_ms(1000);
    }
    return 0;
}
