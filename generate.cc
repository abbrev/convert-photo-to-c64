#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

#include "generate.h"
#include "evaluate.h"

/*
 * Refine one pixel to maximize the score for the whole block.
 */
static bool refine_pixel(const Block *src, Block *gen, int x, int y)
{
	Color& pixel = gen->pixelAt(x, y);

	Color original_color = pixel;
	Color best_color = pixel;
	double best_score = INFINITY;

	for (int i = 0; i < src->getPaletteSize(); ++i) {
		pixel = src->getPalette(i);
		double score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = src->getPalette(i);
		}
	}

	pixel = best_color;
	return best_color != original_color;
}

static bool refine_pixel_by_swapping(const Image *src, Image *gen, int x, int y)
{
	int bx = x / gen->getBlockWidth();
	int by = y / 8;

	const Block& gen_block = gen->blockAt(bx, by);

	bool change = false;

	// try swapping current pixel with neighbors
	for (int yoff = -1; yoff <= 1; ++yoff) {
		for (int xoff = -1; xoff <= 2; ++xoff) {
			if (xoff == 0 && yoff == 0) {
				continue;
			}

			int ox = x + xoff;
			int oy = y + yoff;
			if (ox < 0 || src->getWidth() <= ox ||
			    oy < 0 || src->getHeight() <= oy) {
				continue;
			}

			Color& pixel = gen->pixelAt(x, y);

			int obx = ox / gen->getBlockWidth();
			int oby = oy / 8;
			const Block& other_block = src->blockAt(obx, oby);
			Color& other_pixel = gen->pixelAt(ox, oy);

			if (pixel == other_pixel) {
				// they're the same color; no need to swap
				continue;
			}
			if (&other_block != &gen_block) {
				// TODO check palettes of gen_block and other_block to see if pixel and other_pixel are in both blocks' palettes
				continue;
			}

			double before = evaluate_pixel(src, gen, x, y);

			// swap them
			{
				Color t = other_pixel;
				other_pixel = pixel;
				pixel = t;
			}

			double after = evaluate_pixel(src, gen, x, y);

			if (after > before) {
				// swap them back
				Color t = other_pixel;
				other_pixel = pixel;
				pixel = t;
			} else {
				change = true;
			}
		}
	}

	return change;
}

/*
 * Refine one pixel to maximize the score for the whole block.
 */
static bool refine_pixel(const Image *src, Image *gen, int x, int y)
{
	int bx = x / gen->getBlockWidth();
	int by = y / 8;

	const Block& src_block = src->blockAt(bx, by);

	Color& pixel = gen->pixelAt(x, y);

	Color original_color = pixel;
	Color best_color = pixel;
	double best_score = INFINITY;
	bool change = false;

	change = refine_pixel_by_swapping(src, gen, x, y);

	// try changing the color of just this pixel
	for (int i = 0; i < src_block.getPaletteSize(); ++i) {
		pixel = src_block.getPalette(i);
		double score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = src_block.getPalette(i);
		}
	}
	pixel = best_color;

	if (pixel != original_color) {
		change = true;
	}

	return change;
}

typedef struct {
	uint8_t x, y;
} Coord;

static const Coord *get_shuffled_coords(int width, int height)
{
	static Coord coords[8*8];
	static bool initialized = false;

	if (!initialized) {
		Coord *coord = coords;

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				*coord++ = {(uint8_t)x,(uint8_t)y};
			}
		}
	}

	// shuffle the coords
	for (int i = width*height-1; i > 0; --i) {
		int j = (unsigned long long)rand() * (i+1) / RAND_MAX;
		Coord c   = coords[i];
		coords[i] = coords[j];
		coords[j] = c;
	}
	return coords;
}

// Do up to this many genetic runs.
#define GENETIC_RUNS 100//10000

// If this many consecutive genetic runs yields an unchanging block, return early.
#define MIN_CONSTANT_GENETIC_RUNS 3

/*
 * Genetically refine a block.
 */
void refine_genetically(const Block *src, Block *gen, int passes)
{
	int last_change = 0;

	for (int i = 0; i < passes; ++i) {
		const Coord *coords = get_shuffled_coords(src->getWidth(), 8);

		for (int j = 0; j < src->getWidth()*8; ++j) {
			if (refine_pixel(src, gen, coords[j].x, coords[j].y)) {
				last_change = i;
			}
		}
#if 1
		// If no changes in a few passes, abort and say it's good.
		if (GENETIC_RUNS > MIN_CONSTANT_GENETIC_RUNS && i - last_change >= MIN_CONSTANT_GENETIC_RUNS) {
#if 0
			if (i >= 10) {
				fprintf(stderr, "made it to %d iterations before no change\n", i+1);
			}
#endif
			break;
		}
#if 0
		if (i == GENETIC_RUNS-1) {
			fprintf(stderr, "made it all the way to %d iterations\n", i+1);
		}
#endif
#endif
	}
}

void refine_genetically(const Image *src, Image *gen)
{
	int last_change = 0;
	int runs = GENETIC_RUNS;

	for (int i = 0; i < runs; ++i) {
		fprintf(stderr, "\rrefining   %.2f%%", 100.0 * i / runs);
		const Coord *coords = get_shuffled_coords(gen->getBlockWidth(), 8);

		int num_changes = 0;
		for (int y = 0; y < gen->getHeight(); ++y) {
			for (int x = 0; x < gen->getWidth(); ++x) {
				for (int j = 0; j < gen->getBlockWidth()*8; ++j) {
					int offset = x^y;
					const Coord& coord = coords[(j+offset)%(gen->getBlockWidth()*8)];
					if (refine_pixel(src, gen, x*gen->getBlockWidth()+coord.x, y*8+coord.y)) {
						last_change = i;
						++num_changes;
					}
				}
			}
		}
#if 1
		// If no changes in a few runs, abort and say it's good.
		if (GENETIC_RUNS > MIN_CONSTANT_GENETIC_RUNS && i - last_change >= MIN_CONSTANT_GENETIC_RUNS) {
#if 0
			if (i >= 10) {
				fprintf(stderr, "made it to %d iterations before no change\n", i+1);
			}
#endif
			break;
		}
#if 0
		if (i == GENETIC_RUNS-1) {
			fprintf(stderr, "made it all the way to %d iterations\n", i+1);
		}
#endif
#endif
	}
}

#if 0
/*
 * Generate a block with dither.
 */
static void generate_with_dither(const Block *src, const Color colors[2], Block *gen)
{
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			// TODO
			gen->pixelAt(x, y) = colors[(x^y)&1];
		}
	}
}
#endif

/*
 * Generate a block with the given colors.
 */
void generate(const Block *src, Block *gen)
{
#if 0
	*gen = *src;
#elif 0
	// dither
	Color avg;
	for (int c = 0; c < src->getPaletteSize(); ++c) {
		avg += src->getPalette(c);
	}
	avg /= src->getPaletteSize();

	Color bayer[2][2];

	if (src->getPaletteSize() == 2) {
		const double weight = 0.75;
		bayer[0][0] = (src->getPalette(1) - avg) * +1.000 * weight;
		bayer[0][1] = (src->getPalette(0) - avg) * +0.333 * weight;
		bayer[1][1] = (src->getPalette(1) - avg) * +0.333 * weight;
		bayer[1][0] = (src->getPalette(0) - avg) * +1.000 * weight;
	} else {
		const double weight = 0.75;
		bayer[0][0] = (src->getPalette(0) - avg) * +0.500 * weight;
		bayer[0][1] = (src->getPalette(1) - avg) * +0.500 * weight;
		bayer[1][1] = (src->getPalette(2) - avg) * +0.500 * weight;
		bayer[1][0] = (src->getPalette(3) - avg) * +0.500 * weight;
	}

	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < src->getWidth(); ++x) {
			double best_score = INFINITY;
			for (int c = 0; c < src->getPaletteSize(); ++c) {
				double score = evaluate_color(src->pixelAt(x, y) + bayer[y%2][x%2], src->getPalette(c));
				if (score < best_score) {
					gen->pixelAt(x, y) = src->getPalette(c);
					best_score = score;
				}
			}
		}
	}
#elif 1
	// fill with nearest colors (no dithering)
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < src->getWidth(); ++x) {
			double best_score = INFINITY;
			for (int c = 0; c < src->getPaletteSize(); ++c) {
				double score = evaluate_color(src->pixelAt(x, y), src->getPalette(c));
				if (score < best_score) {
					gen->pixelAt(x, y) = src->getPalette(c);
					best_score = score;
				}
			}
		}
	}
#else
	// fill with first color
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			gen->pixelAt(x, y) = src->getPalette(0);
		}
	}
#endif
}
