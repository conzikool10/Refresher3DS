#pragma once

#include "types.h"

void osk_setup(state_t *state);
void osk_open(uint16_t *title, uint16_t *initial_text);
bool is_osk_running();
char *get_utf8_output();