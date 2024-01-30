#pragma once

#include <stdint.h>

void utf16_to_utf8(const uint16_t *src, uint8_t *dst);
void utf8_to_utf16(const uint8_t *src, uint16_t *dst);