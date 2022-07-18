#ifndef __PINDIR_IDENT_H__
#define __PINDIR_IDENT_H__
#include <stdint.h>

#include "channels.h"
#include "parser.h"

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//limits for automated IO-identification - tweak these if needed
#define PIN_DIR_DETECT_DELAY_MS 1 //delay in millseconds between samples for detection of changing voltages on pin
#define PIN_DIR_DETECT_NB_SAMPLES 3 //nb of samples for detection of changing voltages on pin
#define PIN_DIR_DETECT_DELTA_MAX 50 //max deviation (raw ADC-value) allowed for samples to not be classified as changing (analog or whatever) voltage
#define PIN_DIR_DETECT_HALF_VCC_DELTA_MAX 20 //max deviation (raw ADC-value) around Vcc/2 (ADC_MAX/2) to classify pin as input 
#define PIN_DIR_DETECT_VCC_ADC_MIN 1000 //min raw ADC-value to classify pin as output high or Vcc
#define PIN_DIR_DETECT_GND_ADC_MAX 20 //max raw ADC-value to classifiy pin as output low or GND
#define PIN_DIR_DETECT_PULLUP_ADC_MIN 1020 //min raw ADC-value for avrpin=1 to classify pin as input_pullup
#define PIN_DIR_DETECT_PULLDOWN_ADC_MAX 10 //max raw ADC-value for avrpin=0 to classify pin as input_pulldown

void identify_pin_directions(void);

void print_pin_summary(void);

void cmd_ident(PROTOTYPE_ARGS_HANDLER);
void cmd_chstate(PROTOTYPE_ARGS_HANDLER);

#endif
