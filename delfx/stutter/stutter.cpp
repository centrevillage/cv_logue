#include "common.h"
#include "delayline.hpp"

static dsp::DualDelayLine buffer;

//#define BUFFER_SIZE 262144
#define BUFFER_SIZE 96000

static __sdram f32pair_t buffer_ram[BUFFER_SIZE];

static float loop_length;
static float loop_length_current;
static uint32_t start_point;
static float loop_index;
static float loop_speed;

static const float loop_length_smooth = 0.0f;

#define SPEED_TABLE_SIZE 60
const float speed_table[SPEED_TABLE_SIZE] = {
  -1.0,
  -0.9438743126816936,
  -0.8908987181403392,
  -0.8408964152537145,
  -0.7937005259840999,
  -0.7491535384383406,
  -0.7071067811865476,
  -0.6674199270850173,
  -0.6299605249474365,
  -0.5946035575013605,
  -0.5612310241546866,
  -0.5297315471796475,
  -0.5,
  -0.4719371563408468,
  -0.4454493590701696,
  -0.42044820762685725,
  -0.39685026299204995,
  -0.3745767692191703,
  -0.3535533905932738,
  -0.33370996354250865,
  -0.31498026247371824,
  -0.29730177875068026,
  -0.2806155120773433,
  -0.26486577358982377,
  -0.25,
  0.25,
  0.26486577358982377,
  0.2806155120773433,
  0.29730177875068026,
  0.31498026247371824,
  0.33370996354250865,
  0.3535533905932738,
  0.3745767692191703,
  0.39685026299204995,
  0.42044820762685725,
  0.4454493590701696,
  0.4719371563408468,
  0.5,
  0.5297315471796475,
  0.5612310241546866,
  0.5946035575013605,
  0.6299605249474365,
  0.6674199270850173,
  0.7071067811865476,
  0.7491535384383406,
  0.7937005259840999,
  0.8408964152537145,
  0.8908987181403392,
  0.9438743126816936,
  1.0,
  1.189207115002721,
  1.259921049894873,
  1.3348398541700344,
  1.4142135623730951,
  1.4983070768766815,
  1.5874010519681996,
  1.681792830507429,
  1.7817974362806785,
  1.8877486253633868,
  2.0,
};

void DELFX_INIT(uint32_t platform, uint32_t api) {
  buffer.setMemory(buffer_ram, BUFFER_SIZE);
  loop_length = 0.0f;
  loop_length_current = 0.0f;
  start_point = 0;
  loop_index = 0.0f;
  loop_speed = 1.0f;
}

void DELFX_PROCESS(float *xn, uint32_t frames) {
  float * __restrict x = xn;
  const float * x_e = x + 2*frames;

  if (loop_length > 0.0f) {
    while (x != x_e) {
      loop_length_current = (1.0f - loop_length_smooth) * loop_length + loop_length_smooth * loop_length_current;
      f32pair_t lr = buffer.readFrac(loop_index);
      *x = lr.a;
      *(x+1) = lr.b;
      loop_index -= loop_speed;
      if (loop_index < 0.0f) {
        loop_index += loop_length_current;
      } else if (loop_index >= loop_length) {
        loop_index -= loop_length_current;
      }
      x += 2;
    }
  } else {
    while (x != x_e) {
      buffer.write((f32pair_t){*x, *(x+1)});
      x += 2;
    }
  }
}


void DELFX_PARAM(uint8_t index, int32_t value) {
  const float valf = q31_to_f32(value);
  switch (index) {
  case 0:
    if (valf == 0.0f) {
      loop_index = 0.0f;
      loop_length = 0.0f;
      loop_length_current = 0.0f;
    } else {
      loop_length = clipminf(2048.0f, (BUFFER_SIZE * (1.0f - valf)));
    }
    break;
  case 1:
    loop_speed = speed_table[clipmaxu32(value / (0x7FFFFFFF / SPEED_TABLE_SIZE), SPEED_TABLE_SIZE-1)];
    break;
  default:
    break;
  }
}

