#ifndef IMAGE_H
#define IMAGE_H

#include "block.h"

//#define NUM_COLORS 2
#define BLOCK_WIDTH  8
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

	      Color& pixelAt(int x, int y)       { return blockAt(x/BLOCK_WIDTH, y/BLOCK_HEIGHT).pixels[y%BLOCK_HEIGHT][x%BLOCK_WIDTH]; }
	const Color& pixelAt(int x, int y) const { return blockAt(x/BLOCK_WIDTH, y/BLOCK_HEIGHT).pixels[y%BLOCK_HEIGHT][x%BLOCK_WIDTH]; }

	int getWidth(void) const { return width; }
	int getHeight(void) const { return height; }

private:
	int width, height;
	Block *blocks = nullptr;
};

#endif
