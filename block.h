#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#include "color.h"

// we may support 4x8 blocks in the future
typedef struct {
	Color pixels[8][8];

	      Color& pixelAt(int x, int y)       { return pixels[y][x]; }
	const Color& pixelAt(int x, int y) const { return pixels[y][x]; }

	const Color& getPalette(int c) const { return palette[c]; }
	void setPalette(int c, const Color& color) { palette[c] = color; }

private:
	Color palette[2];
} Block;

#endif
