#ifndef __CHANNELS_H__
#define __CHANNELS_H__
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define NB_CHANNELS_AVAILABLE 24 //do not change for current hardware

//Workaround for crappy levelshifters with defective channels
#define DEFEFCTIVE_CHANNELS 0

typedef enum
{
	MODE_NORMAL,
	MODE_LEVELSHIFTER
} devicemode_t;

typedef enum
{
	PIN_NOT_PROBED=0,
	PIN_IDENT_FAILED, //these pins are ignored but can be manually set to some type
	PIN_CHANGING_VOLTAGE, //ignore these
	PIN_DISABLED, //can be set manually
	PIN_INPUT_FLOATING,
	PIN_INPUT_PULLUP,
	PIN_INPUT_PULLDOWN,
	PIN_OUTPUT_OR_VCC_GND
} pintype_t;

typedef struct
{
	pintype_t type;
	
	uint8_t pos_tdo_pattern;
	
	bool pin_occupied;
} channel_t;

void reset_channels(void);

devicemode_t get_device_mode(void);
bool get_override_inputs(void);
bool get_override_unknown(void);

void cmd_reset(PROTOTYPE_ARGS_HANDLER);
void cmd_devmode(PROTOTYPE_ARGS_HANDLER);
void cmd_channels(PROTOTYPE_ARGS_HANDLER);
void cmd_setmode(PROTOTYPE_ARGS_HANDLER);
void cmd_override(PROTOTYPE_ARGS_HANDLER);

#endif
