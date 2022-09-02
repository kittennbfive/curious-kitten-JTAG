#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "channels.h"
#include "avr_pinfuncs.h"
#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

channel_t channels[NB_CHANNELS_AVAILABLE];
uint8_t nb_channels=0;

#if DEFEFCTIVE_CHANNELS
const bool skip_defective_channels[NB_CHANNELS_AVAILABLE]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //from 0 to NB_CHANNELS_AVAILABLE-1, 1 means defective
#endif

static devicemode_t devmode=MODE_NORMAL;

static bool override_inputs=false;
static bool override_unknown=false;

void reset_channels(void)
{
	uint8_t i;
	for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
	{
		channels[i].type=PIN_NOT_PROBED;
		channels[i].pos_tdo_pattern=0;
		channels[i].pin_occupied=false;
		set_avr_pin_input(i, false);
	}
	
	nb_channels=0;
	override_inputs=false;
	override_unknown=false;
}

devicemode_t get_device_mode(void)
{
	return devmode;
}

bool get_override_inputs(void)
{
	return override_inputs;
}

bool get_override_unknown(void)
{
	return override_unknown;
}

void cmd_reset(PROTOTYPE_ARGS_HANDLER) //0 args //reset everything
{
	ARGS_HANDLER_UNUSED;
	
	reset_channels();
	
	printf_P(PSTR("reset ok"));
}

void cmd_devmode(PROTOTYPE_ARGS_HANDLER)
{
	ARGS_HANDLER_UNUSED;
	
	char str_mode[SZ_BUFFER_ARGUMENTS];
	strcpy(str_mode, get_next_argument());
	
	if(!strcmp(str_mode, "normal"))
	{
		devmode=MODE_NORMAL;
		printf_P(PSTR("device mode set to normal"));
	}
	else if(!strcmp(str_mode, "shift"))
	{
		devmode=MODE_LEVELSHIFTER;
		printf_P(PSTR("device mode set to levelshifter mode, some functions are unavailable"));
	}
	else
		printf_P(PSTR("error: invalid mode"));
}

void cmd_channels(PROTOTYPE_ARGS_HANDLER) //1 arg //set number of channels
{
	ARGS_HANDLER_UNUSED;
	
	nb_channels=atoi(get_next_argument());
	
	if(nb_channels<4 || nb_channels>24)
	{
		printf_P(PSTR("error: invalid number of channels (4-24)"));
		nb_channels=0;
		return;
	}
	
	printf_P(PSTR("number of channels set to %u"), nb_channels);

#if DEFEFCTIVE_CHANNELS
	printf_P(PSTR("\r\nscanning for defective channels...\r\n")); 
	uint8_t ch;
	bool found=false;
	for(ch=0; ch<nb_channels; ch++)
	{
		if(skip_defective_channels[ch])
		{
			found=true;
			printf_P(PSTR("WARNING: channel %u marked as defective, will not be used!\r\n"), ch);
		}
	}
	if(!found)
		printf_P(PSTR("all used channels ok"));
#endif
}

void cmd_setmode(PROTOTYPE_ARGS_HANDLER) //2 args //set channel state / override automatically detected state
{
	ARGS_HANDLER_UNUSED;

	if(get_device_mode()==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode"));
		return;
	}
	
	uint8_t ch=0;
	char state[SZ_BUFFER_ARGUMENTS];
	
	ch=atoi(get_next_argument()); //BUG: if argument is not a number atoi() will return 0 -> channel 0 will be modified
	
	if(ch>23)
	{
		printf_P(PSTR("error: invalid channel (0-23)"));
		return;
	}
	
	strcpy(state, get_next_argument());
	
	if(!strcmp_P(state, PSTR("disabled")))
	{
		channels[ch].type=PIN_DISABLED;
		printf_P(PSTR("channel %u disabled"), ch);
	}
	else if(!strcmp_P(state, PSTR("input")))
	{
		channels[ch].type=PIN_INPUT_FLOATING;
		printf_P(PSTR("channel %u defined as floating input"), ch);
	}
	else if(!strcmp_P(state, PSTR("inputPU")))
	{
		channels[ch].type=PIN_INPUT_PULLUP;
		printf_P(PSTR("channel %u defined as input with pullup"), ch);
	}
	else if(!strcmp_P(state, PSTR("inputPD")))
	{
		channels[ch].type=PIN_INPUT_PULLDOWN;
		printf_P(PSTR("channel %u defined as input with pulldown"), ch);
	}
	else if(!strcmp_P(state, PSTR("output")))
	{
		channels[ch].type=PIN_OUTPUT_OR_VCC_GND;
		printf_P(PSTR("channel %u defined as output"), ch);
	}
	else
		printf_P(PSTR("error: invalid state specified"));
}

void cmd_override(PROTOTYPE_ARGS_HANDLER) //2 args
{
	ARGS_HANDLER_UNUSED;
	
	if(get_device_mode()==MODE_LEVELSHIFTER)
	{
		printf_P(PSTR("error: command unavailable in levelshifter mode"));
		return;
	}
	
	char type[SZ_BUFFER_ARGUMENTS];
	char yesno[SZ_BUFFER_ARGUMENTS];
	
	strcpy(type, get_next_argument());
	strcpy(yesno, get_next_argument());
	
	if(!strcmp_P(type, PSTR("inputs")))
	{
		if(!strcmp_P(yesno, PSTR("yes")))
		{
			override_inputs=true;
			printf_P(PSTR("Will override channels detected as inputs."));
		}
		else if(!strcmp_P(yesno, PSTR("no")))
		{
			override_inputs=false;
			printf_P(PSTR("override input channels set to false"));
		}
		else
			printf_P(PSTR("error: invalid argument (yes/no)"));
	}
	else if(!strcmp_P(type, PSTR("unknown")))
	{
		if(!strcmp_P(yesno, PSTR("yes")))
		{
			override_unknown=true;
			printf_P(PSTR("Will override channels that couldn't be identified.\r\nWARNING: This function is potentially dangerous!"));
		}
		else if(!strcmp_P(yesno, PSTR("no")))
		{
			override_unknown=false;
			printf_P(PSTR("override unknown channels set to false"));
		}
		else
			printf_P(PSTR("error: invalid argument (yes/no)"));
	}
	else
		printf_P(PSTR("error: invalid argument (inputs/unknown)"));
}
