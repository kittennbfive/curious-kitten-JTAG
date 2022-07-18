#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "parser.h"
#include "channels.h"
#include "pindir_ident.h"
#include "jtag.h"
#include "avr_pinfuncs.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

typedef struct
{
	char *cmd;
	uint8_t nb_args_min;
	uint8_t nb_args_max;
	void (*handler)(PROTOTYPE_ARGS_HANDLER);
} command_t;

static char cmd[SZ_BUFFER_COMMAND];
static char args[NB_ARGUMENTS_MAX][SZ_BUFFER_ARGUMENTS];
static uint8_t nb_args=0;
static uint8_t index_args=0;

const command_t commands[]=
{
	{"reset", 0, 0, &cmd_reset}, //channels.c
	{"allinp", 0, 0, &cmd_allinp}, //avr_pinfuncs.c
	{"channels", 1, 1, &cmd_channels}, //channels.c
	{"ident", 0, 0, &cmd_ident}, //pindir_ident.c
	{"chstate", 0, 0, &cmd_chstate}, //pindir_ident.c
	{"setmode", 2, 2, &cmd_setmode}, //channels.c
	{"override", 2, 2, &cmd_override}, //channels.c
	{"tdopullup", 1, 1, &cmd_tdo_pullup}, //jtag.c
	{"jtag1", 0, 2, &cmd_jtag1}, //jtag.c
	{"jtag2", 1, 2, &cmd_jtag2}, //jtag.c
	{"setlevel", 2, 2, &cmd_setlevel}, //avr_pinfuncs.c
	{"taps", 0, 0, &cmd_taps}, //jtag.c
	{"irlen", 0, 0, &cmd_irlen}, //jtag.c
	{"drlen", 0, 0, &cmd_drlen}, //jtag.c
	
	{NULL, 0, 0, NULL}
};

void parse(char * const inp)
{
	nb_args=0;
	index_args=0;
	
	char *ptr;
	
	ptr=strtok(inp, " ");
	if(!ptr)
		return;

	strncpy(cmd, ptr, SZ_BUFFER_COMMAND-1);
	cmd[SZ_BUFFER_COMMAND-1]='\0';
	
	while((ptr=strtok(NULL, " ")) && nb_args<NB_ARGUMENTS_MAX)
	{
		strncpy(args[nb_args], ptr, SZ_BUFFER_ARGUMENTS-1);
		args[nb_args++][SZ_BUFFER_ARGUMENTS-1]='\0';
	}
	
	uint8_t i;
	bool found=false;
	for(i=0; commands[i].cmd; i++)
	{
		if(!strcmp(cmd, commands[i].cmd))
		{
			found=true;
			break;
		}
	}
	if(!found)
	{
		printf_P(PSTR("unknown command %s\r\n"), cmd);
		return;
	}
	
	if(nb_args<commands[i].nb_args_min)
	{
		printf_P(PSTR("not enough arguments for %s\r\n"), cmd);
		return;
	}
	if(nb_args>commands[i].nb_args_max)
	{
		printf_P(PSTR("too many arguments for %s\r\n"), cmd);
		return;
	}
	
	if(!commands[i].handler)
	{
		printf_P(PSTR("no handler specified for command %s - this is a bug\r\n"), cmd);
		return;
	}
	
	commands[i].handler(cmd, nb_args);
	
	printf_P(PSTR("\r\n\r\n"));
}

char * get_next_argument(void)
{
	if(index_args<nb_args)
		return args[index_args++];
	else
		return NULL;
}

char * peek_next_argument(void)
{
	if(index_args<nb_args)
		return args[index_args];
	else
		return NULL;
}

uint8_t get_number_remaining_args(void)
{
	return nb_args-index_args;
}
