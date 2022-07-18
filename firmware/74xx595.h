#ifndef __74XX595_H__
#define __74XX595_H__
#include <stdint.h>

/*
This file is part of curious-kitten-JTAG.

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

void init_595s(void);
void write_data_to_595s(const uint8_t data0, const uint8_t data1);

#endif
