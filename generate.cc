#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

#include "generate.h"
#include "evaluate.h"

static bool refine_pixel_by_swapping(const Block *src, Block *gen, int x, int y)
{
	bool change = false;

	Color& pixel = gen->pixelAt(x, y);

	// try swapping current pixel with neighbors
	for (int yoff = -1; yoff <= 1; ++yoff) {
		for (int xoff = -1; xoff <= 1; ++xoff) {
			if (xoff == 0 && yoff == 0) {
				continue;
			}

			int ox = x + xoff;
			int oy = y + yoff;
			if (ox < 0 || src->getWidth() <= ox ||
			    oy < 0 || src->getHeight() <= oy) {
				continue;
			}

			Color& other_pixel = gen->pixelAt(ox, oy);

			if (pixel == other_pixel) {
				// they're the same color; no need to swap
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
static bool refine_pixel(const Block *src, Block *gen, int x, int y)
{
	Color& pixel = gen->pixelAt(x, y);

	Color original_color = pixel;
	Color best_color = pixel;
	double best_score = INFINITY;
	bool change = false;

	for (int i = 0; i < src->getPaletteSize(); ++i) {
		const Color& p = src->getPalette(i);
		pixel = p;
		double score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = p;
		}
	}

	pixel = best_color;

	if (pixel != original_color) {
		change = true;
	}

	change |= refine_pixel_by_swapping(src, gen, x, y);

	return change;
}

static bool color_in_palette(const Color& c, const Block& block)
{
	for (int i = 0; i < block.getPaletteSize(); ++i) {
		const Color& p = block.getPalette(i);
		if (c == p) {
			return true;
		}
	}
	return false;
}

static bool refine_pixel_by_swapping(const Image *src, Image *gen, int x, int y)
{
	int bx = x / gen->getBlockWidth();
	int by = y / 8;

	const Block& gen_block = gen->blockAt(bx, by);

	bool change = false;

	Color& pixel = gen->pixelAt(x, y);

	// try swapping current pixel with neighbors
	for (int yoff = -1; yoff <= 1; ++yoff) {
		for (int xoff = -1; xoff <= 1; ++xoff) {
			if (xoff == 0 && yoff == 0) {
				continue;
			}

			int ox = x + xoff;
			int oy = y + yoff;
			if (ox < 0 || src->getWidth() <= ox ||
			    oy < 0 || src->getHeight() <= oy) {
				continue;
			}

			int obx = ox / gen->getBlockWidth();
			int oby = oy / 8;
			const Block& other_block = src->blockAt(obx, oby);
			Color& other_pixel = gen->pixelAt(ox, oy);

			if (pixel == other_pixel) {
				// they're the same color; no need to swap
				continue;
			}
			// if pixels fall in different blocks, it's ok as long as both pixels' colors are found in both blocks' palettes
			if (&other_block != &gen_block &&
			    (!color_in_palette(pixel, other_block) || !color_in_palette(other_pixel, gen_block))) {
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

	// try changing the color of just this pixel
	for (int i = 0; i < src_block.getPaletteSize(); ++i) {
		const Color& p = src_block.getPalette(i);
		pixel = p;
		double score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = p;
		}
	}
	pixel = best_color;

	if (pixel != original_color) {
		change = true;
	}

	change |= refine_pixel_by_swapping(src, gen, x, y);

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

/*
 * Genetically refine a block.
 */
void refine_genetically(const Block *src, Block *gen, int passes)
{
	int last_change = 0;

	for (int i = 0; i < passes; ++i) {
		const Coord *coords = get_shuffled_coords(src->getWidth(), src->getHeight());

		for (int j = 0; j < src->getWidth()*src->getHeight(); ++j) {
			if (refine_pixel(src, gen, coords[j].x, coords[j].y)) {
				last_change = i;
			}
		}
#if 1
		// If no changes in a few passes, abort and say it's good.
		if (i - last_change >= 1) {
#if 0
			if (i >= 10) {
				fprintf(stderr, "made it to %d iterations before no change\n", i+1);
			}
#endif
			break;
		}
#if 0
		if (i == passes-1) {
			fprintf(stderr, "made it all the way to %d iterations\n", i+1);
		}
#endif
#endif
	}
}

// If this many consecutive genetic runs yields an unchanging block, return early.
#define MIN_CONSTANT_GENETIC_RUNS 3

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
				for (int j = 0; j < gen->getBlockWidth()*gen->getBlockHeight(); ++j) {
					int offset = x^y;
					const Coord& coord = coords[(j+offset)%(gen->getBlockWidth()*gen->getBlockHeight())];
					if (refine_pixel(src, gen, x*gen->getBlockWidth()+coord.x, y*gen->getBlockHeight()+coord.y)) {
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
	for (int y = 0; y < src->getHeight(); ++y) {
		for (int x = 0; x < src->getWidth(); ++x) {
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
	// Floyd-Steinberg dither
	Block errors;
	errors.setWidth(gen->getWidth());
	// TODO
#elif 1
	// fill with alternating colors from the palette
	const Coord *coords = get_shuffled_coords(src->getWidth(), src->getHeight());
	for (int j = 0; j < src->getWidth()*src->getHeight(); ++j) {
		gen->pixelAt(coords[j].x, coords[j].y) = src->getPalette(j % src->getPaletteSize());
	}
#elif 1
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
	for (int y = 0; y < src->getHeight(); ++y) {
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
	for (int y = 0; y < src->getHeight(); ++y) {
		for (int x = 0; x < src->getWidth(); ++x) {
			gen->pixelAt(x, y) = src->getPalette(0);
		}
	}
#endif
}
