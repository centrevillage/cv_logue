extern "C" {
#include "common.h"
#include "common_table.h"
#include "userosc.h"
#include "wavetable.h"
#include "delay.h"
#include "quantizer.h"
}

#include "simplelfo.hpp"

typedef struct State {
  float w0; // current delta phase for update
  float w1; // current delta phase for update
  float phase0; // current phase for osc1
  float phase1; // current phase for osc2
  const int16_t* wt0; // wavetable 0
  const int16_t* wt1; // wavetable 1
  float wt_balance; // mix balance for wavetable0&1 [0-1.0]
  float lfo;  // current lfo value
  float lfoz;
  uint8_t pitch_diff;
  float sample_rate;
  float sample_rate_random;
  float sample_phase;
  float phase_lfo_speed;
  float sig;
} State;

static float delay_feed = 0.0f;
static const float delay_enhance_amp = 1.0f;

static State state;
static dsp::SimpleLFO phase_lfo;

__fast_inline float wavetable_value_get(const int16_t* p_wt, float phase) {
  const float p = phase - (uint32_t)phase;
  const float x0f = p * WAVETABLE_SAMPLE_SIZE;
  const uint32_t x0 = ((uint32_t)x0f) & (WAVETABLE_SAMPLE_SIZE-1);
  const uint32_t x1 = (x0 + 1) & (WAVETABLE_SAMPLE_SIZE-1);
  const float value0 = ((float)p_wt[x0] / (float)0x7FFF);
  const float value1 = ((float)p_wt[x1] / (float)0x7FFF);
  return linintf(x0f - (uint32_t)x0f, value0, value1);
}

__fast_inline float wavetable_osc_get(float phase0, float phase1, float lfoz) {
  const float wave0 = wavetable_value_get(state.wt0, phase0);
  const float wave1 = wavetable_value_get(state.wt1, phase1);
  const float mix = clip01f(state.wt_balance+lfoz);
  return ((1.0f - mix) * wave0) + (mix * wave1);
}

void delay_init() {
  delay_setup();
  delay_divide_set(4);
  delay_feed = 0.48f;
}

static const float s_fs_recip = 1.f / 48000.f;

void OSC_INIT(uint32_t platform, uint32_t api) {
  state.w0 = 0.f;
  state.w1 = 0.f;
  state.phase0 = 0.f;
  state.phase1 = 0.f;
  state.wt0 = &(wavetables[0][0]);
  state.wt1 = &(wavetables[1][0]);
  state.wt_balance = 0.0f;
  state.lfo = 0.0f;
  state.lfoz = 0.0f;
  state.pitch_diff = 0;
  state.sample_rate = 1.0f;
  state.sample_rate_random = 0.0f;
  state.sample_phase = 0.0f;
  state.sig = 0.0f;
  state.phase_lfo_speed = 0.0f;
  phase_lfo.reset();

  delay_init();
}

// params: oscillator parameter
// yn: write address
// frames: requested frame count for write
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) {
  // convert pitch(1 octave = 12 * 256 pitch value) to delta phase
  uint16_t pitch0 = params->pitch;
  uint16_t pitch1 = params->pitch;
  if (state.pitch_diff) {
    pitch1 = ((uint16_t)quantizer_process(pitch0 >> 8, state.pitch_diff) << 8);
  }
  //uint16_t pitch1 = params->pitch + (((uint16_t)state.pitch_diff) << 8);
  const float w0 = state.w0 = osc_w0f_for_note(pitch0 >> 8, pitch0 & 0xFF);
  const float w1 = state.w1 = osc_w0f_for_note(pitch1 >> 8, pitch1 & 0xFF);
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  state.lfo = q31_to_f32(params->shape_lfo);
  float lfoz = state.lfoz;
  const float lfo_inc = (state.lfo - lfoz) / frames;
  float phase0 = state.phase0;
  float phase1 = state.phase1;
  float sample_phase = state.sample_phase;

  for (; y != y_e; ) {
    sample_phase += state.sample_rate + (osc_white() * state.sample_rate * state.sample_rate_random);
    phase_lfo.cycle();
    float phase1_mod = phase1;
    if (state.phase_lfo_speed > 0.0f) {
      phase1_mod = phase1 + phase_lfo.sine_uni();
      phase1_mod -= (uint32_t)phase1_mod;
    }
    float sig;
    if (sample_phase >= 1.0f) {
      // calculate current signal value
      sig = osc_softclipf(0.05f, wavetable_osc_get(phase0, phase1_mod, lfoz));
      state.sig = sig;
    } else {
      sig = state.sig;
    }

    sig += delay_value_get() * delay_feed / delay_enhance_amp;
    delay_value_add(osc_softclipf(0.05f, sig * delay_enhance_amp));
    // convert Floating-Point[-1, 1] to Fixed-Point[-0x7FFFFFFF, 0x7FFFFFFF] and update signal
    *(y++) = f32_to_q31(sig);

    sample_phase -= (uint32_t)sample_phase;

    // inclement wave phase
    phase0 += w0;
    phase1 += w1;
    // reset wave phase if it overlap
    phase0 -= (uint32_t)phase0;
    phase1 -= (uint32_t)phase1;

    lfoz += lfo_inc;
  }
  state.sample_phase = sample_phase;
  state.phase0 = phase0;
  state.phase1 = phase1;
  state.lfoz = lfoz;
}

void OSC_NOTEON(const user_osc_param_t * const params) {
  quantizer_note_pitch_record(params->pitch >> 8);
  quantizer_scale_map_reconstruct();
}

void OSC_NOTEOFF(const user_osc_param_t * const params) { (void)params; }

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch (index) {
  case k_osc_param_id1:
    // wave
    state.wt0 = &(wavetables[(value & 0b11)][0]);
    state.wt1 = &(wavetables[(value & 0b11)][0]);
    break;
  case k_osc_param_id2:
    // diff
    // pitch differenct osc2 from osc1
    state.pitch_diff = value;
  case k_osc_param_id3:
    // blur
    delay_feed = clipmaxf((float)value * 0.01f, 0.95f);
    break;
  case k_osc_param_id4:
    // collapse
    // sample rate reduction
    state.sample_rate = (1.0f - clipmaxf(common_logf((float)value / 100.0f), 1.0f)) * 0.96f + 0.04f;
    state.sample_rate_random = 0.004f * (float)value;
  case k_osc_param_id5:
    // phase lfo
    state.phase_lfo_speed = clipmaxf((float)value * 0.01f, 1.0f);
    phase_lfo.setF0(state.phase_lfo_speed, s_fs_recip);
  case k_osc_param_id6:
    // melt
    // cross fade before note on pitch to current note on pitch (osc1, osc2)
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

