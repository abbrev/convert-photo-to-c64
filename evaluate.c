#include <stdio.h>

#include "evaluate.h"

long evaluate_color(const Color *src, const Color *gen)
{
	return ((long)src->r - gen->r) * ((long)src->r - gen->r) +
	       ((long)src->g - gen->g) * ((long)src->g - gen->g) +
	       ((long)src->b - gen->b) * ((long)src->b - gen->b);
}

// evaluate a single pixel
long evaluate_single_pixel(const Block *src, const Block *gen, int x, int y)
{
	if (x < 0 || 8 <= x ||
	    y < 0 || 8 <= y) return 0;

	const Color *srcColor = &src->pixels[y][x];

	// compute weighted average color using neighboring pixels
	// XXX coords can be removed by refactoring the loop and weights below
	static const struct { int x, y; } coords[9] = {
		{-1,-1},{ 0,-1},{ 1,-1},
		{-1, 0},{ 0, 0},{ 1, 0},
		{-1, 1},{ 0, 1},{ 1, 1},
	};

	const int weights[9] = {
		 1, 5, 1,
		 5, 8, 5,
		 1, 5, 1,
	};
	const int weightsum = 32;

	int rsum = 0;
	int gsum = 0;
	int bsum = 0;
	for (int i = 0; i < 9; ++i) {
		int xx = x + coords[i].x;
		int yy = y + coords[i].y;

		// mirror at the edges
		if (xx < 0) xx = -xx;
		else if (xx >= 8) xx = 15 - xx - 1;
		if (yy < 0) yy = -yy;
		else if (yy >= 8) yy = 15 - yy - 1;

		rsum += (int)gen->pixels[yy][xx].r * weights[i];
		gsum += (int)gen->pixels[yy][xx].g * weights[i];
		bsum += (int)gen->pixels[yy][xx].b * weights[i];
	}

	Color avgColor = {
		rsum / weightsum,
		gsum / weightsum,
		bsum / weightsum,
	};

	return evaluate_color(srcColor, &avgColor);
}

// evaluate a pixel and its neighbors that are affected by it
// apparently this doesn't work very well....
long evaluate_pixel(const Block *src, const Block *gen, int x, int y)
{
	long score = 0;
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			score += evaluate_single_pixel(src, gen, x+xx, y+yy);
		}
	}
	return score;
}

long evaluate(const Block *src, const Block *gen)
{
	long error = 0;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			error += evaluate_single_pixel(src, gen, x, y);
		}
	}
	return error;
}

#if 0
static const Block blockA = {
{
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
}
};

static const Block blockB = {
{
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
}
};

static const Block blockC = {
{
{{255,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
{{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},{  0,  0,  0},},
{{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},},
}
};

int main()
{
	fprintf(stderr, "evaluate(A, A) = %ld\n", evaluate(&blockA, &blockA));
	fprintf(stderr, "evaluate(A, B) = %ld\n", evaluate(&blockA, &blockB));
	fprintf(stderr, "evaluate(B, A) = %ld\n", evaluate(&blockB, &blockA));
	fprintf(stderr, "evaluate(B, B) = %ld\n", evaluate(&blockB, &blockB));
	fprintf(stderr, "evaluate(A, C) = %ld\n", evaluate(&blockA, &blockC));

	return 0;
}
#endif
