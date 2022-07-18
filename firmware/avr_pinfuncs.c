#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>

#include "avr_pinfuncs.h"
#include "channels.h"
#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

extern channel_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c

static const channelpin_t channel_to_avr_pin[NB_CHANNELS_AVAILABLE]={ {&DDRA, &PORTA, &PINA, (1<<0)}, {&DDRA, &PORTA, &PINA, (1<<1)}, {&DDRA, &PORTA, &PINA, (1<<2)}, {&DDRA, &PORTA, &PINA, (1<<3)}, {&DDRA, &PORTA, &PINA, (1<<4)}, {&DDRA, &PORTA, &PINA, (1<<5)}, {&DDRD, &PORTD, &PIND, (1<<6)}, {&DDRD, &PORTD, &PIND, (1<<7)}, {&DDRB, &PORTB, &PINB, (1<<0)}, {&DDRB, &PORTB, &PINB, (1<<1)}, {&DDRB, &PORTB, &PINB, (1<<2)}, {&DDRB, &PORTB, &PINB, (1<<3)}, {&DDRB, &PORTB, &PINB, (1<<4)}, {&DDRB, &PORTB, &PINB, (1<<5)}, {&DDRB, &PORTB, &PINB, (1<<6)}, {&DDRB, &PORTB, &PINB, (1<<7)}, {&DDRC, &PORTC, &PINC, (1<<0)}, {&DDRC, &PORTC, &PINC, (1<<1)}, {&DDRC, &PORTC, &PINC, (1<<2)}, {&DDRC, &PORTC, &PINC, (1<<3)}, {&DDRC, &PORTC, &PINC, (1<<4)}, {&DDRC, &PORTC, &PINC, (1<<5)}, {&DDRC, &PORTC, &PINC, (1<<6)}, {&DDRC, &PORTC, &PINC, (1<<7)} };

//always inline these for speed during JTAG-search!

inline __attribute__((__always_inline__)) void set_avr_pin_input(const uint8_t ch, const bool pullup)
{
	if(pullup)
		(*channel_to_avr_pin[ch].port)|=channel_to_avr_pin[ch].bitmask;
	else
		(*channel_to_avr_pin[ch].port)&=~channel_to_avr_pin[ch].bitmask;
	(*channel_to_avr_pin[ch].ddr)&=~channel_to_avr_pin[ch].bitmask;
}

inline __attribute__((__always_inline__)) void set_avr_pin_output(const uint8_t ch, const bool value)
{
	(*channel_to_avr_pin[ch].ddr)|=channel_to_avr_pin[ch].bitmask;
	if(value)
		(*channel_to_avr_pin[ch].port)|=channel_to_avr_pin[ch].bitmask;
	else
		(*channel_to_avr_pin[ch].port)&=~channel_to_avr_pin[ch].bitmask;
}

inline __attribute__((__always_inline__)) bool read_avr_pin(const uint8_t ch)
{
	return !!((*channel_to_avr_pin[ch].pin)&channel_to_avr_pin[ch].bitmask);
}

void cmd_setlevel(PROTOTYPE_ARGS_HANDLER) //2 args
{
	ARGS_HANDLER_UNUSED;
	
	uint8_t ch;
	char level_str[SZ_BUFFER_ARGUMENTS];
	bool level;
	
	ch=atoi(get_next_argument()); //BUG: if argument is not a number atoi() will return 0 -> channel 0 will be modified
	
	if(ch>23)
	{
		printf_P(PSTR("error: invalid channel (0-23)"));
		return;
	}
	
	if(channels[ch].type!=PIN_INPUT_FLOATING && channels[ch].type!=PIN_INPUT_PULLUP && channels[ch].type!=PIN_INPUT_PULLDOWN)
	{
		printf_P(PSTR("error: channel %u not set as input on DUT"), ch);
		return;
	}
	
	strcpy(level_str, get_next_argument());
	
	if(strlen(level_str)>1 || (level_str[0]!='0' && level_str[0]!='1'))
	{
		printf_P(PSTR("error: invalid level (0/1)"));
		return;
	}
	
	level=level_str[0]-'0';
	
	set_avr_pin_output(ch, level);
	
	printf_P(PSTR("channel %u set to %u"), ch, level);
}

void cmd_allinp(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	uint8_t i;
	for(i=0; i<NB_CHANNELS_AVAILABLE; i++)
		set_avr_pin_input(i, false);
	
	printf_P(PSTR("all channels set to input on AVR"));
}
