#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "generate.h"
#include "evaluate.h"

/*
 * Refine one pixel to maximize the score for the whole block.
 */
static bool refine_pixel_genetically(const Block *src, const Color colors[2], Block *gen, int x, int y)
{
	bool change = false;

	Color best_color = gen->pixels[y][x];
	double best_score = evaluate_pixel(src, gen, x, y);
	for (int i = 0; i < 2; ++i) {
		if (best_score == 0) {
			goto perfect;
		}
		if (colors[i].r == best_color.r &&
		    colors[i].g == best_color.g &&
		    colors[i].b == best_color.b) {
			continue;
		}
		gen->pixels[y][x] = colors[i];
		double score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = colors[i];
			change = true;
		}
	}

	gen->pixels[y][x] = best_color;
perfect:
	return change;
}

typedef struct {
	uint8_t x, y;
} Coord;

static const Coord *get_shuffled_coords(void)
{
	static Coord coords[8*8];
	static bool initialized = false;

	if (!initialized) {
		Coord *coord = coords;

		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				*coord++ = (Coord){x,y};
			}
		}
	}

	// shuffle the coords
	for (int i = 8*8-1; i > 0; --i) {
		int j = (unsigned long long)rand() * (i+1) / RAND_MAX;
		Coord c   = coords[i];
		coords[i] = coords[j];
		coords[j] = c;
	}
	return coords;
}

// Do up to this many genetic runs.
#define GENETIC_RUNS 1000

// If this many consecutive genetic runs yields an unchanging block, return early.
#define MIN_CONSTANT_GENETIC_RUNS 3

/*
 * Genetically refine a block.
 */
static void refine_genetically(const Block *src, const Color colors[2], Block *gen)
{
	int last_change = 0;

	for (int i = 0; i < GENETIC_RUNS; ++i) {
		const Coord *coords = get_shuffled_coords();

		for (int j = 0; j < 8*8; ++j) {
			if (refine_pixel_genetically(src, colors, gen, coords[j].x, coords[j].y)) {
				last_change = i;
			}
		}
#if 1
		// If no changes in a few runs, abort and say it's good.
		if (i - last_change >= MIN_CONSTANT_GENETIC_RUNS) {
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

/*
 * Generate a block with dither.
 */
static void generate_with_dither(const Block *src, const Color colors[2], Block *gen)
{
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			// TODO
			gen->pixels[y][x] = (x&1)^(y&1) ? colors[0] : colors[1];
		}
	}
}

/*
 * Generate a block with the given colors.
 */
void generate(const Block *src, const Color colors[2], Block *gen)
{
	// fill with nearest colors (no dithering)
	bool perfect_score = true;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			double best_score = LONG_MAX;
			for (int c = 0; c < 2; ++c) {
				double score = evaluate_color(&src->pixels[y][x], &colors[c]);
				if (score < best_score) {
					gen->pixels[y][x] = colors[c];
					best_score = score;
				}
			}
			if (best_score != 0) {
				perfect_score = false;
			}
		}
	}
	if (perfect_score) {
		// If this whole block matches the given colors exactly, we
		// don't have to refine it.
		return;
	}
	//generate_with_dither(src, colors, gen);
	refine_genetically(src, colors, gen);
}
