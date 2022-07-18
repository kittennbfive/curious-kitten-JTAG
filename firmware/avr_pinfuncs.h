#ifndef __AVR_PINFUNCS_H__
#define __AVR_PINFUNCS_H__
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

typedef struct
{
	volatile uint8_t * ddr;
	volatile uint8_t * port;
	volatile uint8_t * pin;
	uint8_t bitmask;
} channelpin_t;

void set_avr_pin_input(const uint8_t ch, const bool pullup);
void set_avr_pin_output(const uint8_t ch, const bool value);
bool read_avr_pin(const uint8_t ch);

void cmd_setlevel(PROTOTYPE_ARGS_HANDLER);
void cmd_allinp(PROTOTYPE_ARGS_HANDLER);

#endif
