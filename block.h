#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#include "color.h"

typedef struct {
	const Color& pixelAt(int x, int y) const { return pixels[y][x]; }
	      Color& pixelAt(int x, int y)       { return pixels[y][x]; }

	const Color& getPalette(int c) const { return palette[c]; }
	void setPalette(int c, const Color& color) { palette[c] = color; }

	int getPaletteSize() const { return paletteSize; }
	void setPaletteSize(int size) { paletteSize = size; }

	int getWidth() const { return width; }
	void setWidth(int w) { width = w; }

	int getHeight() const { return 8; }

private:
	Color pixels[8][8];

	int width = 8;
	int paletteSize;
	Color palette[4];
} Block;

#endif
