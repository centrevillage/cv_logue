#include "quantizer.h"

#define QUANTIZER_SCALE_KEY_MAX 6

uint8_t quantizer_scale_keys[QUANTIZER_SCALE_KEY_MAX] = {0, 0, 0, 0, 0, 0};
// 対応するindexのkeyで参照すると移動量が帰ってくる
int8_t quantizer_scale_map[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t quantizer_key_to_scale[12];
uint8_t quantizer_scale_keys_idx = 0;
uint8_t quantizer_scale_key_min = 11;
uint8_t quantizer_scale_key_max = 0;
uint8_t quantizer_scale_changed = 0;

void quantizer_note_pitch_record(uint8_t note_num) {
  uint8_t scale_key = note_num % 12;
  for (uint8_t i=0;i<QUANTIZER_SCALE_KEY_MAX;++i) {
    if (quantizer_scale_keys[i] == scale_key) {
      return;
    }
  }

  quantizer_scale_changed = 1;
  quantizer_scale_keys[quantizer_scale_keys_idx] = scale_key;
  quantizer_scale_keys_idx = (quantizer_scale_keys_idx + 1) % QUANTIZER_SCALE_KEY_MAX;
}

void quantizer_scale_map_reconstruct() {
  if (!quantizer_scale_changed) {
    return;
  }
  // reconstruct quantizer_scale_map
  for (uint8_t i=0; i<12; ++i) {
    quantizer_scale_map[i] = 127;
  }
  quantizer_scale_key_min = 11;
  quantizer_scale_key_max = 0;
  for (uint8_t i=0;i<QUANTIZER_SCALE_KEY_MAX;++i) {
    quantizer_scale_map[quantizer_scale_keys[i]] = 0;
    if (quantizer_scale_keys[i] < quantizer_scale_key_min) {
      quantizer_scale_key_min = quantizer_scale_keys[i];
    }
    if (quantizer_scale_keys[i] > quantizer_scale_key_max) {
      quantizer_scale_key_max = quantizer_scale_keys[i];
    }
  }
  uint8_t found_127 = 1;
  while (found_127) {
    found_127 = 0;
    for (uint8_t i=0;i<12;++i) {
      if (quantizer_scale_map[i] != 127) {
        uint8_t left = (i+11)%12;
        uint8_t right = (i+1)%12;
        if (quantizer_scale_map[left] == 127) {
          quantizer_scale_map[left] = quantizer_scale_map[i]+1;
        } else if (quantizer_scale_map[right] == 127) {
          quantizer_scale_map[right] = quantizer_scale_map[i]-1;
        }
      } else {
        found_127 = 1;
      }
    }
  }
  quantizer_scale_changed = 0;
}

uint8_t quantizer_process(uint8_t note_num, int8_t shift) {
  int16_t v = (int16_t)note_num + shift;
  if (v < quantizer_scale_key_min) {
    v = quantizer_scale_key_min;
  } else if (v > 108 + quantizer_scale_key_max) {
    v = 108 + quantizer_scale_key_max;
  }

  int8_t key = v % 12;
  int8_t base = v - key;

  int8_t move = quantizer_scale_map[key];
  return (uint8_t)(v + move);
}
