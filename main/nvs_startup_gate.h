#pragma once

#include <stdbool.h>

/* Initialize NVS without erasing or retrying. False means app startup must stop. */
bool panel_nvs_startup_gate(void);
