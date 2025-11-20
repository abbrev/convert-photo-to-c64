#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#include "color.h"

// we may support 4x8 blocks in the future
typedef struct {
	Color pixels[8][8];
} Block;

#endif
