#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define BAUD 115200
#include <util/setbaud.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "74xx595.h"
#include "4067.h"
#include "adc.h"
#include "pindir_ident.h"
#include "channels.h"
#include "avr_pinfuncs.h"
#include "jtag.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

Please read the fine manual!


Fuses using avrdude: -U lfuse:w:0xff:m -U hfuse:w:0xd9:m
*/

int uart_putchar(char c, FILE *stream);

#define SZ_UI_BUFFER 25

#define ECHO_INPUT 1

int main(void)
{	
	//init UART
	//respect write order!
	UBRRH=UBRR_VALUE>>8;
	UBRRL=UBRR_VALUE&0xff;
	UCSRB=(1<<TXEN)|(1<<RXEN);
	
	FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
	stdout = &uart_output;
	
	//ADC set to ADC7, make this configurable later for OCP DUT
	ADC_init();
	
	//SW-SPI
	init_595s();
	
	init_find_jtag();
	
	reset_channels();
	
	printf_P(PSTR("\r\n\r\nThis is curious-kitten-JTAG version 1\r\n(c)2022 by kittennbfive\r\nAGPLv3+ and NO WARRANTY - USE AT YOUR OWN RISK!\r\nPlease read the fine manual.\r\n\r\n"));
	
	printf_P(PSTR("command? "));
	
	char cmd[SZ_UI_BUFFER];
	char rx;
	uint8_t pos=0;
	
	while(1)
	{
		while(!(UCSRA&(1<<RXC)));
		rx=UDR;
		if(rx=='\r' || rx=='\n')
		{
			cmd[pos]='\0';
#if ECHO_INPUT
			printf_P(PSTR("\r\n"));
#endif
			parse(cmd);
			pos=0;
			printf_P(PSTR("command? "));
		}
		else if(pos<SZ_UI_BUFFER-1)
		{
#if ECHO_INPUT
			uart_putchar(rx, NULL);
#endif
			cmd[pos++]=rx;
		}
	}
	
	return 0;
}

int uart_putchar(char c, FILE *stream)
{
	(void)stream;
	
	while(!(UCSRA&(1<<UDRE)));
	UDR=c;
	
	return 0;
}
