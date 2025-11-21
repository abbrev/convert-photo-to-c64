#ifndef IMAGE_H
#define IMAGE_H

#include <assert.h>

#include "block.h"

//#define NUM_COLORS 2
#define BLOCK_HEIGHT 8

class Image {
public:
	Image(int width, int height) {
		init(width, height);
	}

	// copy constructor
	Image(const Image& other) {
		init(other.width, other.height);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				blockAt(x, y) = other.blockAt(x, y);
			}
		}
	}

	void init(int width, int height) {
		blocks = new Block[width * height];
		this->width = width;
		this->height = height;
	}

	~Image() {
		delete[] blocks;
	}

	      Block& blockAt(int x, int y)       { return blocks[y*width + x]; }
	const Block& blockAt(int x, int y) const { return blocks[y*width + x]; }

	      Color& pixelAt(int x, int y)       { assert(blockWidth!=0); return blockAt(x/blockWidth, y/BLOCK_HEIGHT).pixelAt(x%blockWidth, y%BLOCK_HEIGHT); }
	const Color& pixelAt(int x, int y) const { assert(blockWidth!=0); return blockAt(x/blockWidth, y/BLOCK_HEIGHT).pixelAt(x%blockWidth, y%BLOCK_HEIGHT); }

	int getWidth(void) const { return width; }
	int getHeight(void) const { return height; }

	int getBlockWidth() const { return blockWidth; }
	void setBlockWidth(int w) { blockWidth = w; }

private:
	int width, height;
	int blockWidth;
	Block *blocks = nullptr;
};

#endif
