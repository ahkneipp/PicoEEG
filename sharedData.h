#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdbool.h>

extern volatile float* validBuffer;
extern volatile bool newData;
extern volatile unsigned long long int validChunkTimestamp_us;

#endif
