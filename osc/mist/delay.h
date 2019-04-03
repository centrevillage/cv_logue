#ifndef _DELAY_H_
#define _DELAY_H_

#include "common.h"

#define DELAY_SIZE 8192

void delay_setup();
void delay_value_add(float value);
float delay_value_get();
void delay_divide_set(uint8_t divide);

#endif /* _DELAY_H_ */
