#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

typedef struct {
	uint8_t r,g,b;
} Color;

// we may support 4x8 blocks in the future
typedef struct {
	Color pixels[8][8];
} Block;

#endif
