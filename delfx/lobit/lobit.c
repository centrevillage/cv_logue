#include "common.h"
#include "delay.h"

static uint8_t delay_pinpon = 0;
static float delay_feed = 0.0f;
static const float delay_enhance_amp = 8.0f;

void DELFX_INIT(uint32_t platform, uint32_t api) {
  delay_setup();
  delay_divide_set(1);
  delay_feed = 0.48f;
#ifdef PINPON_DELAY
  delay_pinpon = 1;
#endif
}

void DELFX_PROCESS(float *xn, uint32_t frames) {
  float * __restrict x = xn;
  const float * x_e = x + 2*frames;

  while (x != x_e) {
    if (delay_pinpon) {
      *x += delay_right_value_get() * delay_feed / delay_enhance_amp;
      *(x+1) += delay_left_value_get() * delay_feed / delay_enhance_amp;
    } else {
      *x += delay_left_value_get() * delay_feed / delay_enhance_amp;
      *(x+1) += delay_right_value_get() * delay_feed / delay_enhance_amp;
    }
    delay_value_add(fx_softclipf(0.05f, (*x) * delay_enhance_amp), fx_softclipf(0.05f, (*(x+1)) * delay_enhance_amp));
    x += 2;
  }
}


void DELFX_PARAM(uint8_t index, int32_t value) {
  const float valf = q31_to_f32(value);
  switch (index) {
  case 0:
    delay_divide_set(1 + (uint8_t)(valf * 31.0f));
    break;
  case 1:
    delay_feed = clip01f(0.05f + valf);
    break;
  default:
    break;
  }
}

