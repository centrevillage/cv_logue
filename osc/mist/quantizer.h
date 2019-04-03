#ifndef _QUANTIZER_H_
#define _QUANTIZER_H_

#include "common.h"

void quantizer_note_pitch_record(uint8_t note_num);
uint8_t quantizer_process(uint8_t note_num, int8_t shift);
void quantizer_scale_map_reconstruct();

#endif /* _QUANTIZER_H_ */
