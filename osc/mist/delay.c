#include "delay.h"

static int8_t delay_buffer[DELAY_SIZE];
volatile uint16_t delay_buffer_end_index = 0;
volatile uint16_t delay_buffer_read_index = 0;

volatile uint8_t delay_sample_index = 0;
volatile uint8_t delay_sample_divide = 1;

void delay_setup() {
  for (int i=0;i<DELAY_SIZE;++i) {
    delay_buffer[i] = 0;
  }
  delay_buffer_read_index = 0;
  delay_buffer_end_index = 0;
}

void delay_value_add(float value) {
  if ((delay_sample_index++ % delay_sample_divide) == 0) {
    delay_buffer[delay_buffer_end_index] = (int8_t)(clip1m1f(value) * 127.0f);
    delay_buffer_end_index = (delay_buffer_end_index + 1) % DELAY_SIZE;
    delay_buffer_read_index = (delay_buffer_end_index + 1) % DELAY_SIZE;
  }
}

float delay_value_get() {
  float rate = (float)(delay_sample_index % delay_sample_divide) / (float)delay_sample_divide;
  if (rate == 0.0f) {
    return (float)delay_buffer[delay_buffer_read_index] / 127.0f;
  }
  return ((float)delay_buffer[delay_buffer_read_index] / 127.0f * (1.0f-rate))
    + ((float)delay_buffer[(delay_buffer_read_index+1)%DELAY_SIZE] / 127.0f * rate);
}

void delay_divide_set(uint8_t divide) {
  if (divide > 32) {
    divide = 32;
  } else if (divide == 0) {
    divide = 1;
  }
  delay_sample_divide = divide;
}
