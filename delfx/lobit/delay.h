#ifndef _LOBIT_DELAY_H_
#define _LOBIT_DELAY_H_

#include "common.h"

#define DELAY_SIZE 3072

void delay_setup();
void delay_value_add(float left, float right);
float delay_left_value_get();
float delay_right_value_get();
void delay_divide_set(uint8_t divide);

#endif /* _LOBIT_DELAY_H_ */
