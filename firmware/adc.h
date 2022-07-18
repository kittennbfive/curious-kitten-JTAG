#ifndef __ADC_H__
#define __ADC_H__
#include <stdint.h>

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define ADC_MAX 1023 //do not change

void ADC_init(void);
uint16_t ADC_sample(void);

#endif
