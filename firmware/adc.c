#include <avr/io.h>
#include <stdint.h>

#include "adc.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

void ADC_init(void)
{
	ADMUX=(1<<REFS0)|(1<<MUX2)|(1<<MUX1)|(1<<MUX0); //ref AVCC, ADC7 single ended input
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1); //prescaler 64 == 115kHz ADC clock for 7,3728MHz XTAL
}

uint16_t ADC_sample(void)
{
	ADCSRA|=(1<<ADSC);
	while(ADCSRA&(1<<ADSC));
	uint8_t adcl, adch;
	//respect read order!
	adcl=ADCL;
	adch=ADCH;
	return ((uint16_t)adch<<8)|adcl;
}
