#include "common.h"
#include "userosc.h"
#include "wavetable.h"

typedef struct State {
  float w0; // current delta phase for update
  float phase; // current phase
  const int16_t* wt0; // wavetable 0
  const int16_t* wt1; // wavetable 1
  float wt_balance; // mix balance for wavetable0&1 [0-1.0]
  float lfo;  // current lfo value
  float lfoz;
} State;

static State state;

__fast_inline float wavetable_value_get(const int16_t* p_wt, float phase) {
  const float p = phase - (uint32_t)phase;
  const float x0f = p * WAVETABLE_SAMPLE_SIZE;
  const uint32_t x0 = ((uint32_t)x0f) & (WAVETABLE_SAMPLE_SIZE-1);
  const uint32_t x1 = (x0 + 1) & (WAVETABLE_SAMPLE_SIZE-1);
  const float value0 = ((float)p_wt[x0] / (float)0x7FFF);
  const float value1 = ((float)p_wt[x1] / (float)0x7FFF);
  return linintf(x0f - (uint32_t)x0f, value0, value1);
}

__fast_inline float wavetable_osc_get(float phase, float lfoz) {
  const float wave0 = wavetable_value_get(state.wt0, phase);
  const float wave1 = wavetable_value_get(state.wt1, phase);
  const float mix = clip01f(state.wt_balance+lfoz);
  return ((1.0f - mix) * wave0) + (mix * wave1);
}

void OSC_INIT(uint32_t platform, uint32_t api) {
  state.w0 = 0.f;
  state.phase = 0.f;
  state.wt0 = &(wavetables[0][0]);
  state.wt1 = &(wavetables[1][0]);
  state.wt_balance = 0.0f;
  state.lfo = 0.0f;
  state.lfoz = 0.0f;
}

// params: oscillator parameter
// yn: write address
// frames: requested frame count for write
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) {
  // convert pitch(1 octave = 12 * 256 pitch value) to delta phase
  const float w0 = state.w0 = osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF);
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  state.lfo = q31_to_f32(params->shape_lfo);
  float lfoz = state.lfoz;
  const float lfo_inc = (state.lfo - lfoz) / frames;
  float phase = state.phase;
  for (; y != y_e; ) {
    // calculate current signal value
    const float sig = osc_softclipf(0.05f, 0.5f * wavetable_osc_get(phase, lfoz));
    // convert Floating-Point[-1, 1] to Fixed-Point[-0x7FFFFFFF, 0x7FFFFFFF] and update signal
    *(y++) = f32_to_q31(sig);
    // inclement wave phase
    phase += w0;
    // reset wave phase if it overlap
    phase -= (uint32_t)phase;

    lfoz += lfo_inc;
  }
  state.phase = phase;
  state.lfoz = lfoz;
}

void OSC_NOTEON(const user_osc_param_t * const params) { (void)params; }

void OSC_NOTEOFF(const user_osc_param_t * const params) { (void)params; }

void OSC_PARAM(uint16_t index, uint16_t value) {
  const float valf = param_val_to_f32(value);
  
  switch (index) {
  case k_osc_param_id1:
    state.wt0 = &(wavetables[(value & 0b111) * 2][0]);
    state.wt1 = &(wavetables[(value & 0b111) * 2 + 1][0]);
    break;
  case k_osc_param_id2:
  case k_osc_param_id3:
  case k_osc_param_id4:
  case k_osc_param_id5:
  case k_osc_param_id6:
    break;
  case k_osc_param_shape:
    state.wt_balance = param_val_to_f32(value);
    break;
  case k_osc_param_shiftshape:
    break;
  default:
    break;
  }
}

