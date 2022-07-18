#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/cpufunc.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "jtag.h"
#include "channels.h"
#include "avr_pinfuncs.h"
#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define LENGTH_PATTERN 8

static const bool detection_pattern[LENGTH_PATTERN]={1,0,0,1,1,1,1,0};

extern channel_t channels[NB_CHANNELS_AVAILABLE]; //declared in channels.c
extern uint8_t nb_channels; //declared in channels.c

jtag_pins_t jtag_pins;

uint8_t nb_taps=0;
uint8_t irlen=0;

static bool tdo_pullup=false;

//TODO: add adjustable delay for slow DUT?
static void clock_pulses(const uint8_t tck, uint16_t nb)
{
	while(nb--)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		set_avr_pin_output(tck, 0);
		_NOP();
	}
}

static void send_tms_command(const uint8_t tms, const uint8_t tck, char const * const cmd)
{
	uint8_t i=0;
	while(cmd[i])
	{
		set_avr_pin_output(tms, cmd[i]-'0');
		clock_pulses(tck, 1);
		i++;
	}
	
	set_avr_pin_output(tms, 0);
}

static void send_ir(const uint8_t tdi, const uint8_t tms, const uint8_t tck, uint32_t instr, uint8_t length_ir)
{
	while(length_ir)
	{
		if(instr&1)
			set_avr_pin_output(tdi, 1);
		else
			set_avr_pin_output(tdi, 0);
		if(length_ir==1)
			set_avr_pin_output(tms, 1);
		else
			set_avr_pin_output(tms, 0);
		clock_pulses(tck, 1);
		instr>>=1;
		length_ir--;
	}
}

static uint32_t nCr(const uint8_t n, const uint8_t r)
{
	uint32_t res=1;
	uint8_t i;
	for(i=1; i<=r; i++)
		res=res*(n+1-i)/i;
	return res;
}

void init_find_jtag(void)
{
	jtag_pins.valid_data=false;
}

bool find_jtag1(const bool ignore_if_pullup_missing, const bool dontstop, const bool silent)
{
	uint8_t i,j;
	
	uint8_t tdi, tck, tms, tdo;
	
	bool override_inputs=get_override_inputs();
	bool override_unknown=get_override_unknown();
	
	bool found=false;
	
	if(!silent)
		printf_P(PSTR("searching...\r\n"));
	
	for(tdi=0; tdi<nb_channels; tdi++)
	{
		if(channels[tdi].pin_occupied)
			continue;
		
		if(channels[tdi].type!=PIN_INPUT_FLOATING && channels[tdi].type!=PIN_INPUT_PULLUP && channels[tdi].type!=PIN_INPUT_PULLDOWN && !(channels[tdi].type==PIN_IDENT_FAILED && override_unknown))
			continue;
		
		if(ignore_if_pullup_missing && channels[tdi].type!=PIN_INPUT_PULLUP)
			continue;
		
		set_avr_pin_output(tdi, 1);
		
		for(tck=0; tck<nb_channels; tck++)
		{
			if(tck==tdi)
				continue;
			
			if(channels[tck].pin_occupied)
				continue;
			
			if(channels[tck].type!=PIN_INPUT_FLOATING && channels[tck].type!=PIN_INPUT_PULLUP && channels[tck].type!=PIN_INPUT_PULLDOWN && !(channels[tck].type==PIN_IDENT_FAILED && override_unknown))
				continue;
			
			set_avr_pin_output(tck, 0);
			
			for(tms=0; tms<nb_channels; tms++)
			{
				if(tms==tck || tms==tdi)
					continue;
				
				if(channels[tms].pin_occupied)
					continue;
				
				if(channels[tms].type!=PIN_INPUT_FLOATING && channels[tms].type!=PIN_INPUT_PULLUP && channels[tms].type!=PIN_INPUT_PULLDOWN && !(channels[tms].type==PIN_IDENT_FAILED && override_unknown))
					continue;
				
				if(ignore_if_pullup_missing && channels[tms].type!=PIN_INPUT_PULLUP)
					continue;
				
				set_avr_pin_output(tms, 1);
				
				//reset
				clock_pulses(tck, NB_CLK_PULSES_RESET);
				
				//select IR
				send_tms_command(tms, tck, "01100");
				
				//send bypass-instr = all ones
				set_avr_pin_output(tdi, 1);
				clock_pulses(tck, NB_CLK_PULSES_BYPASS_INSTR);
				
				//go via update_ir to idle
				send_tms_command(tms, tck, "110");				
				
				//select DR
				send_tms_command(tms, tck, "100");
				
				for(tdo=0; tdo<nb_channels; tdo++)
					channels[tdo].pos_tdo_pattern=0;
				
				//shift detection pattern several times because there might be several TAP in the chain
				for(i=0; i<CYCLES_SHIFT_DETECTION_PATTERN; i++)
				{
					for(j=0; j<LENGTH_PATTERN; j++)
					{
						set_avr_pin_output(tdi, detection_pattern[j]);
						
						clock_pulses(tck, 1);
												
						for(tdo=0; tdo<nb_channels; tdo++)
						{
							if(tdo==tdi || tdo==tck || tdo==tms)
								continue;
								
							if(channels[tdo].type!=PIN_OUTPUT_OR_VCC_GND && !override_inputs && !override_unknown)
								continue;
							
							set_avr_pin_input(tdo, tdo_pullup);
							
							bool value_tdo=read_avr_pin(tdo);
							
							set_avr_pin_input(tdo, false);
							
							if(value_tdo==detection_pattern[channels[tdo].pos_tdo_pattern])
							{
								channels[tdo].pos_tdo_pattern++;
								if(channels[tdo].pos_tdo_pattern==LENGTH_PATTERN)
								{
									found=true;
									
									printf_P(PSTR("JTAG FOUND: tdi %u tck %u tms %u tdo %u"), tdi, tck, tms, tdo);
									if(dontstop)
										printf_P(PSTR("\r\n"));

									jtag_pins.ch_tdi=tdi;
									jtag_pins.ch_tck=tck;
									jtag_pins.ch_tms=tms;
									jtag_pins.ch_tdo=tdo;
									jtag_pins.valid_data=true;
									
									for(i=0; i<nb_channels; i++)
										set_avr_pin_input(i, false);
									
									if(!dontstop)
										return true;
								}
							}
							else
								channels[tdo].pos_tdo_pattern=0;
						}
					}
				}
				set_avr_pin_input(tms, false);
			}
			set_avr_pin_input(tck, false);
		}
		set_avr_pin_input(tdi, false);
	}
	
	if(!found)
	{
		if(!silent)
			printf_P(PSTR("nothing found using jtag1"));
		return false;
	}
	else
		return true;
}

typedef struct
{
	uint8_t channel;
	bool * ptr_pin_occupied;
	bool has_pullup;
	bool has_pulldown;
} input_pin_t;

void find_jtag2(const uint8_t nb_inputs_to_wiggle, const bool ignore_if_pullup_missing)
{
	input_pin_t all_input_pins[NB_CHANNELS_AVAILABLE];
	uint8_t nb_input_pins=0;
	
	bool override_unknown=get_override_unknown();
	
	uint8_t i;
	
	for(i=0; i<nb_channels; i++)
	{
		switch(channels[i].type)
		{
			case PIN_INPUT_FLOATING:
				all_input_pins[nb_input_pins].channel=i;
				all_input_pins[nb_input_pins].has_pullup=false;
				all_input_pins[nb_input_pins].has_pulldown=false;
				all_input_pins[nb_input_pins++].ptr_pin_occupied=&(channels[i].pin_occupied);
				break;
			
			case PIN_INPUT_PULLUP:
				all_input_pins[nb_input_pins].channel=i;
				all_input_pins[nb_input_pins].ptr_pin_occupied=&(channels[i].pin_occupied);
				all_input_pins[nb_input_pins].has_pullup=true;
				all_input_pins[nb_input_pins++].has_pulldown=false;
				break;

			case PIN_INPUT_PULLDOWN:
				all_input_pins[nb_input_pins].channel=i;
				all_input_pins[nb_input_pins].ptr_pin_occupied=&(channels[i].pin_occupied);
				all_input_pins[nb_input_pins].has_pullup=false;
				all_input_pins[nb_input_pins++].has_pulldown=true;
				break;
			
			case PIN_IDENT_FAILED:
				if(override_unknown)
				{
					all_input_pins[nb_input_pins].channel=i;
					all_input_pins[nb_input_pins].has_pullup=false;
					all_input_pins[nb_input_pins].has_pulldown=false;
					all_input_pins[nb_input_pins++].ptr_pin_occupied=&(channels[i].pin_occupied);
				}
				break;
			
			default: break;
		}
	}
	
	uint32_t tries_max=nCr(nb_input_pins, nb_inputs_to_wiggle)*(1UL<<nb_inputs_to_wiggle);
	
	printf_P(PSTR("%u inputs known, wiggling %u pins at once, %lu tries at most\r\n"), nb_input_pins, nb_inputs_to_wiggle, tries_max);
	
	if(nb_inputs_to_wiggle>nb_input_pins)
	{
		printf_P(PSTR("error: number inputs to wiggle is > nb available inputs"));
		return;
	}
	
	uint8_t positions[MAX_INPUT_PINS_TO_WIGGLE];
	uint8_t index_positions=nb_inputs_to_wiggle-1;
	
	for(i=0; i<nb_inputs_to_wiggle; i++)
		positions[i]=i;
	
	bool running=true;
	
	uint32_t current_combination=0;
	uint32_t combi_shift=0;
	uint32_t compare_mask=(1<<nb_inputs_to_wiggle);
	bool bit=0;
	bool skip=false;
	
	while(running)
	{
		for(current_combination=0; !(current_combination&compare_mask); current_combination++)
		{
			skip=false;
			for(i=0, combi_shift=current_combination; i<nb_inputs_to_wiggle; i++, combi_shift>>=1)
			{
				bit=combi_shift&1;
				
				//if pin has a pullup/down we don't need to test while pulling it high/low -> skip to speed things up
				if((bit && all_input_pins[positions[i]].has_pullup) ||
				(!bit && all_input_pins[positions[i]].has_pulldown))
				{
					skip=true;
					break;
				}
			}
			if(skip)
				continue;
			
			for(i=0, combi_shift=current_combination; i<nb_inputs_to_wiggle; i++, combi_shift>>=1)
			{
				*(all_input_pins[positions[i]].ptr_pin_occupied)=true; //mark pin/channel as used so jtag1() won't touch it
				set_avr_pin_output(all_input_pins[positions[i]].channel, combi_shift&1);
			}
			
			if(find_jtag1(ignore_if_pullup_missing, false, true))
			{
				printf_P(PSTR("\r\nstate wiggled pins:\r\n"));
				
				for(i=0, combi_shift=current_combination; i<nb_inputs_to_wiggle; i++, combi_shift>>=1)
					printf_P(PSTR("\tchannel %u output %u\r\n"), all_input_pins[positions[i]].channel, combi_shift&1);
				
				for(i=0; i<nb_channels; i++)
					set_avr_pin_input(i, false);
				
				return;
			}
		}
		
		for(i=0; i<nb_inputs_to_wiggle; i++)
		{
			*(all_input_pins[positions[i]].ptr_pin_occupied)=false;
			set_avr_pin_input(all_input_pins[positions[i]].channel, false);
		}
		
		if(positions[index_positions]<nb_input_pins-1)
			positions[index_positions]++;
		else
		{
			while(index_positions && positions[index_positions]>=nb_input_pins-1)
			{
				positions[index_positions]=0;
				index_positions--;
				positions[index_positions]++;
			}
			
			for(i=index_positions; i<nb_inputs_to_wiggle-1; i++)
			{
				if(positions[i+1]<=positions[i])
					positions[i+1]=positions[i]+1;
			}
			
			index_positions=nb_inputs_to_wiggle-1;
			
			if(positions[0]==nb_input_pins-1)
				running=false;
		}
	}
	
	printf_P(PSTR("no luck using jtag2"));
}

void get_nb_taps(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo)
{
	set_avr_pin_input(tdo, false);
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);
	
	//select IR
	send_tms_command(tms, tck, "01100");
	
	//send bypass-instr = all ones
	set_avr_pin_output(tdi, 1);
	clock_pulses(tck, NB_CLK_PULSES_BYPASS_INSTR);
	
	//go via update_ir to idle
	send_tms_command(tms, tck, "110");
	
	//select DR
	send_tms_command(tms, tck, "100");
	
	//clock in a single 0
	set_avr_pin_output(tdi, 0);	
	clock_pulses(tck, 1);
	set_avr_pin_output(tdi, 1);
	
	//wait for this 0
	bool found=false;
	uint16_t i;
	bool pinlevel;
	for(i=0; i<NB_CLK_PULSES_MAX_GET_NB_TAPS; i++)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		pinlevel=read_avr_pin(tdo);
		set_avr_pin_output(tck, 0);
		_NOP();
		if(!pinlevel)
		{
			found=1;
			break;
		}
	}

	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);

	if(found)
		nb_taps=i+1;
	else
		nb_taps=0;
}

void get_length_ir(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo)
{
	set_avr_pin_input(tdo, false);
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);
	
	//select IR
	send_tms_command(tms, tck, "01100");
	
	//fill the IR with 1
	set_avr_pin_output(tdi, 1);
	clock_pulses(tck, NB_CLK_PULSES_FILL_IR);

	//clock in a single 0
	set_avr_pin_output(tdi, 0);
	clock_pulses(tck, 1);
	
	//wait for this 0
	bool found=false;
	uint8_t i;
	bool pinlevel;
	for(i=0; i<NB_CLK_PULSES_MAX_GET_LENGTH_IR; i++)
	{
		set_avr_pin_output(tck, 1);
		_NOP();
		pinlevel=read_avr_pin(tdo);
		set_avr_pin_output(tck, 0);
		_NOP();
		if(!pinlevel)
		{
			found=1;
			break;
		}
	}
	
	//reset
	set_avr_pin_output(tms, 1);	
	clock_pulses(tck, NB_CLK_PULSES_RESET);

	if(found)
		irlen=i+1;
	else
		irlen=0;
}

void probe_dr_lengths(const uint8_t tms, const uint8_t tdi, const uint8_t tck, const uint8_t tdo) //DANGEROUS! Read the manual!
{	
	set_avr_pin_input(tdo, false);
	
	//go to idle
	send_tms_command(tms, tck, "0");
	
	uint32_t instr;
	uint16_t i;
	bool found;
	
	for(instr=0x00; instr<(1U<<irlen); instr++)
	{
		printf_P(PSTR("instruction 0x%lx: "), instr);
		
		if(instr==(1U<<irlen)-1)
			printf_P(PSTR("last one, BYPASS, length DR should be 1: "));
		
		//select and write IR
		send_tms_command(tms, tck, "1100");
		send_ir(tdi, tms, tck, instr, irlen);
		
		//select DR
		send_tms_command(tms, tck, "1100");
		
		//fill the current DR with 1
		set_avr_pin_output(tdi, 1);
		clock_pulses(tck, NB_CLK_PULSES_FILL_DR);

		//clock in a single 0
		set_avr_pin_output(tdi, 0);
		clock_pulses(tck, 1);
		
		//wait for this 0
		found=false;
		bool pinlevel;
		for(i=0; i<NB_CLK_PULSES_MAX_GET_LENGTH_DR; i++)
		{
			set_avr_pin_output(tck, 1);
			_NOP();
			pinlevel=read_avr_pin(tdo);
			set_avr_pin_output(tck, 0);
			_NOP();
			if(!pinlevel)
			{
				printf_P(PSTR("length DR is %u\r\n"), i+1);
				found=true;
				break;
			}
		}
		
		if(!found)
			printf_P(PSTR("length DR not found, too big?\r\n"));

		//go back to idle
		send_tms_command(tms, tck, "110");
	}
}

void cmd_jtag1(PROTOTYPE_ARGS_HANDLER) //0-2 args
{
	(void)cmd;
	
	if(nb_channels==0)
	{
		printf_P(PSTR("error: number of channels is 0"));
		return;
	}
	
	uint8_t ch;
	bool found_output=false;
	for(ch=0; ch<nb_channels; ch++)
	{
		if(channels[ch].type==PIN_OUTPUT_OR_VCC_GND)
		{
			found_output=true;
			break;
		}
	}
	if(!found_output && !get_override_inputs())
	{
		printf_P(PSTR("error: no outputs on DUT detected, need at least one for TDO"));
		return;
	}
	
	bool ignore_if_pullup_missing=false;
	bool dontstop=false;
	
	char arg[SZ_BUFFER_ARGUMENTS];
	
	uint8_t nb=nb_args;
	while(nb--)
	{
		strcpy(arg, get_next_argument());
		if(!strcmp_P(arg, PSTR("ignore")))
		{
			printf_P(PSTR("will ignore inputs without needed pullup\r\n"));
			ignore_if_pullup_missing=true;
			
		}
		else if(!strcmp_P(arg, PSTR("dontstop")))
		{
			printf_P(PSTR("won't stop once JTAG found\r\n"));
			dontstop=true;
		}
		else
		{
			printf_P(PSTR("error: invalid argument"));
			return;
		}
	}
	
	find_jtag1(ignore_if_pullup_missing, dontstop, false);
}

void cmd_jtag2(PROTOTYPE_ARGS_HANDLER) //1-2 args
{
	(void)cmd;
	
	if(nb_channels==0)
	{
		printf_P(PSTR("error: number of channels is 0"));
		return;
	}
	
	uint8_t ch;
	bool found_output=false;
	for(ch=0; ch<nb_channels; ch++)
	{
		if(channels[ch].type==PIN_OUTPUT_OR_VCC_GND)
		{
			found_output=true;
			break;
		}
	}
	if(!found_output && !get_override_inputs())
	{
		printf_P(PSTR("error: no outputs on DUT detected, need at least one for TDO"));
		return;
	}
	
	uint8_t nb_pins_to_wiggle=0;
	
	bool ignore_if_pullup_missing=false;
	
	nb_pins_to_wiggle=atoi(get_next_argument());
	if(nb_pins_to_wiggle==0)
	{
		printf_P(PSTR("error: number of inputs to wiggle is 0, use jtag1 in this case"));
		return;
	}
	
	if(nb_args==2 && !strcmp_P(get_next_argument(), PSTR("ignore")))
	{
		printf_P(PSTR("will ignore inputs without needed pullup\r\n"));
		ignore_if_pullup_missing=true;
	}
	
	find_jtag2(nb_pins_to_wiggle, ignore_if_pullup_missing);
}

void cmd_taps(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known"));
		return;
	}
	
	get_nb_taps(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
	
	printf_P(PSTR("found %u TAP(s)"), nb_taps);
}

void cmd_irlen(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known"));
		return;
	}
	
	if(nb_taps!=1)
	{
		printf_P(PSTR("error: irlen will only work if there is a single TAP"));
		return;
	}
	
	get_length_ir(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
	
	if(irlen)
		printf_P(PSTR("length of IR is %u bits"), irlen);
	else
		printf_P(PSTR("could not find length of IR, too big?"));
}

void cmd_drlen(PROTOTYPE_ARGS_HANDLER) //0 args
{
	ARGS_HANDLER_UNUSED;
	
	if(!jtag_pins.valid_data)
	{
		printf_P(PSTR("error: no JTAG-interface known"));
		return;
	}
	
	if(nb_taps!=1)
	{
		printf_P(PSTR("error: irlen will only work if there is a single TAP"));
		return;
	}
	
	if(irlen==0)
	{
		printf_P(PSTR("error: length of IR is not known"));
		return;
	}
	
	probe_dr_lengths(jtag_pins.ch_tms, jtag_pins.ch_tdi, jtag_pins.ch_tck, jtag_pins.ch_tdo);
}


void cmd_tdo_pullup(PROTOTYPE_ARGS_HANDLER) //1 arg
{
	ARGS_HANDLER_UNUSED;
	
	char yesno[SZ_BUFFER_ARGUMENTS];
	
	strcpy(yesno, get_next_argument());
	
	if(!strcmp_P(yesno, PSTR("yes")))
	{
		tdo_pullup=true;
		printf_P(PSTR("Pullup on TDO enabled."));
	}
	else if(!strcmp_P(yesno, PSTR("no")))
	{
		tdo_pullup=false;
		printf_P(PSTR("Pullup on TDO disabled."));
	}
	else
		printf_P(PSTR("error: invalid argument (yes/no)"));
}
