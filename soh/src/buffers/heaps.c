#include "z64.h"

// 0x38000 bytes
u8 gAudioHeap[0x38000] __attribute__ ((aligned (16)));

//u8 gSystemHeap[UNK_SIZE];
u8 gSystemHeap[1024 * 1024 * 128] __attribute__ ((aligned (16)));
