#pragma once
#include <stdbool.h>
#include <stddef.h>

void athom_secure_zero(void *buffer, size_t size);
bool athom_text_contains_secret(const char *text, const char *secret);
const char *athom_status_name(int status);
