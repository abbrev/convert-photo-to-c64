#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include "generate.h"
#include "evaluate.h"

/*
 * Refine one pixel to maximize the score for the whole block.
 */
static void refine_pixel_genetically(const Block *src, const Color colors[2], Block *gen, int x, int y)
{
	gen->pixels[y][x] = colors[0];

	int best_color = 0;
	long best_score = evaluate_pixel(src, gen, x, y);
	for (int i = 0; i < 2; ++i) {
		gen->pixels[y][x] = colors[i];
		long score = evaluate_pixel(src, gen, x, y);
		if (score < best_score) {
			best_score = score;
			best_color = i;
			if (score == 0) {
				goto perfect;
			}
		}
	}

perfect:
	gen->pixels[y][x] = colors[best_color];
}

#define GENETIC_RUNS 3

typedef struct {
	uint8_t x, y;
} Coord;

static void generate_shuffled_coords(Coord coords[8*8*GENETIC_RUNS])
{
	Coord *coord = coords;

	for (int i = 0; i < GENETIC_RUNS; ++i) {
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				*coord++ = (Coord){x,y};
			}
		}
	}

	// shuffle the coordinates
	for (int i = 8*8*GENETIC_RUNS-1; i > 0; --i) {
		int j = (unsigned long long)rand() * (i+1) / RAND_MAX;
		Coord c   = coords[i];
		coords[i] = coords[j];
		coords[j] = c;
	}
}

/*
 * Genetically refine a block.
 */
static void refine_genetically(const Block *src, const Color colors[2], Block *gen)
{
	static bool shuffle_initialized = false;
	static Coord coords[8*8*GENETIC_RUNS];

	if (!shuffle_initialized
#if 1
	    // use on-the-fly shuffled coordinates
	    // without this, blocks tend to have repeating patterns

	    || true
#endif
	   ) {
		generate_shuffled_coords(coords);
		shuffle_initialized = true;
	}

	for (int i = 0; i < 8*8*GENETIC_RUNS; ++i) {
		refine_pixel_genetically(src, colors, gen, coords[i].x, coords[i].y);
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
	bool perfect_score = true;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			long best_score = LONG_MAX;
			for (int c = 0; c < 2; ++c) {
				long score = evaluate_color(&src->pixels[y][x], &colors[c]);
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
