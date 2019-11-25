#ifndef COMMON_TABLE_H_
#define COMMON_TABLE_H_

#include "common.h"

#define COMMON_TABLE_SIZE 1
#define COMMON_TABLE_SAMPLE_SIZE 256

typedef enum {
  CMN_TBL_TYP_LOG = 0
} CommonTableType;

extern const float common_tables[COMMON_TABLE_SIZE][COMMON_TABLE_SAMPLE_SIZE];

static inline __attribute__((optimize("Ofast"), always_inline))
float common_logf(float x) {
  float pos = x * (float)(COMMON_TABLE_SAMPLE_SIZE-1);
  uint16_t x0 = pos;
  float rate = pos - (float)x0;
  if (rate == 0.0f) {
    return common_tables[CMN_TBL_TYP_LOG][x0];
  }
  return linintf(rate, common_tables[CMN_TBL_TYP_LOG][x0], common_tables[CMN_TBL_TYP_LOG][x0+1]);
}

#endif /* COMMON_TABLE_H_ */
