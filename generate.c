#include <stdlib.h>

#include "generate.h"
#include "evaluate.h"

/*
 * Refine one pixel to maximize the score for the whole block.
 */
static long refine_pixel_genetically(const Block *src, const Color colors[2], Block *gen, int x, int y)
{
	gen->pixels[y][x] = colors[0];

	int best_color = 0;
	int best_score = evaluate(src, gen);
	for (int i = 0; i < 2; ++i) {
		gen->pixels[y][x] = colors[i];
		int score = evaluate(src, gen);
		if (score < best_score) {
			best_score = score;
			best_color = i;
		}
	}

	gen->pixels[y][x] = colors[best_color];

	return best_score;
}

#define GENETIC_RUNS 3

/*
 * Genetically refine a block.
 */
static long refine_genetically(const Block *src, const Color colors[2], Block *gen)
{
	long score = 0;
#if 1
	for (int i = 0; i < GENETIC_RUNS * 64; ++i) {
		int x = rand()%8;
		int y = rand()%8;
		score = refine_pixel_genetically(src, colors, gen, x, y);
	}
#else
	// TODO pick pixels at random
	for (int i = 0; i < GENETIC_RUNS; ++i) {
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				score = refine_pixel_genetically(src, colors, gen, x, y);
			}
		}
	}
#endif
	return score;
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
 *
 * Return evaluation score.
 */
long generate(const Block *src, const Color colors[2], Block *gen)
{
	generate_with_dither(src, colors, gen);
	return refine_genetically(src, colors, gen);
}
