#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "evaluate.h"

double evaluate_color(const Color *src, const Color *gen)
{
	return ((src->r - gen->r) * (src->r - gen->r) +
	        (src->g - gen->g) * (src->g - gen->g) +
	        (src->b - gen->b) * (src->b - gen->b));
}

#define DITHER_KERNEL_SIZE2 (DITHER_KERNEL_SIZE*DITHER_KERNEL_SIZE)

#if 1
#define DITHER_KERNEL_SIZE 3
static const int dither_pattern[DITHER_KERNEL_SIZE2] = {
	1, 8, 1,
	8, 1, 8,
	1, 8, 1,
};

static double eval_weights[DITHER_KERNEL_SIZE2] = {
	0,0,0,
	0,1,0,
	0,0,0,
};
#else
#define DITHER_KERNEL_SIZE 5
static const int dither_pattern[DITHER_KERNEL_SIZE2] = {
	0, 1, 1, 1, 0,
	1, 0, 9, 0, 1,
	1, 9, 0, 9, 1,
	1, 0, 9, 0, 1,
	0, 1, 1, 1, 0,
};

static int eval_weights[DITHER_KERNEL_SIZE2] = {
	0,0,0,0,0,
	0,0,0,0,0,
	0,0,1,0,0,
	0,0,0,0,0,
	0,0,0,0,0,
};
#endif

#define CENTER_WEIGHT 10000 // some large value so small dither weights can be represented
static double eval_weightsum = 1;

void set_dither_weight(double w)
{
	int total_dither_weight = 0;
	for (int i = 0; i < DITHER_KERNEL_SIZE2; ++i) {
		total_dither_weight += dither_pattern[i];
	}
	eval_weightsum = 0;
	for (int i = 0; i < DITHER_KERNEL_SIZE2; ++i) {
		eval_weights[i] = (i == DITHER_KERNEL_SIZE2/2) ? CENTER_WEIGHT : (double)dither_pattern[i] * CENTER_WEIGHT * w / total_dither_weight;
		eval_weightsum += eval_weights[i];
	}
}

// evaluate a single pixel
double evaluate_single_pixel(const Block *src, const Block *gen, int x, int y)
{
	if (x < 0 || src->getWidth() <= x ||
	    y < 0 || 8 <= y) return 0;

	const Color *srcColor = &src->pixels[y][x];

	// compute weighted average color using neighboring pixels
	// XXX coords can be removed by refactoring the loop and weights below
#if DITHER_KERNEL_SIZE == 3
	static const struct { int x, y; } coords[DITHER_KERNEL_SIZE2] = {
		{-1,-1},{ 0,-1},{ 1,-1},
		{-1, 0},{ 0, 0},{ 1, 0},
		{-1, 1},{ 0, 1},{ 1, 1},
	};
#else
	static const struct { int x, y; } coords[DITHER_KERNEL_SIZE2] = {
		{-2,-2},{-1,-2},{ 0,-2},{ 1,-2},{ 2,-2},
		{-2,-1},{-1,-1},{ 0,-1},{ 1,-1},{ 2,-1},
		{-2, 0},{-1, 0},{ 0, 0},{ 1, 0},{ 2, 0},
		{-2, 1},{-1, 1},{ 0, 1},{ 1, 1},{ 2, 1},
		{-2, 2},{-1, 2},{ 0, 2},{ 1, 2},{ 2, 2},
	};
#endif

	double rsum = 0;
	double gsum = 0;
	double bsum = 0;
	for (int i = 0; i < DITHER_KERNEL_SIZE2; ++i) {
		int xx = x + coords[i].x;
		int yy = y + coords[i].y;

		// mirror at the edges
		if (xx < 0) xx = -xx;
		else if (xx >= src->getWidth()) xx = 2*(src->getWidth() - 1) - xx;
		if (yy < 0) yy = -yy;
		else if (yy >= 8) yy = 2*(8-1) - yy;

		rsum += gen->pixels[yy][xx].r * eval_weights[i];
		gsum += gen->pixels[yy][xx].g * eval_weights[i];
		bsum += gen->pixels[yy][xx].b * eval_weights[i];
	}

	rsum /= eval_weightsum;
	gsum /= eval_weightsum;
	bsum /= eval_weightsum;

	if (rsum < 0) rsum = 0;
	else if (rsum > 1) rsum = 1;
	if (gsum < 0) gsum = 0;
	else if (gsum > 1) gsum = 1;
	if (bsum < 0) bsum = 0;
	else if (bsum > 1) bsum = 1;

	Color avgColor = {
		rsum,
		gsum,
		bsum,
		0,
	};

	return evaluate_color(srcColor, &avgColor);
}

// evaluate a single pixel
double evaluate_single_pixel(const Image *src, const Image *gen, int x, int y)
{
	if (x < 0 || gen->getBlockWidth()*gen->getWidth()  <= x ||
	    y < 0 || 8*gen->getHeight() <= y) return 0;

	const Color *srcColor = &src->pixelAt(x, y);

	// compute weighted average color using neighboring pixels
	// XXX coords can be removed by refactoring the loop and weights below
#if DITHER_KERNEL_SIZE == 3
	static const struct { int x, y; } coords[DITHER_KERNEL_SIZE2] = {
		{-1,-1},{ 0,-1},{ 1,-1},
		{-1, 0},{ 0, 0},{ 1, 0},
		{-1, 1},{ 0, 1},{ 1, 1},
	};
#else
	static const struct { int x, y; } coords[DITHER_KERNEL_SIZE2] = {
		{-2,-2},{-1,-2},{ 0,-2},{ 1,-2},{ 2,-2},
		{-2,-1},{-1,-1},{ 0,-1},{ 1,-1},{ 2,-1},
		{-2, 0},{-1, 0},{ 0, 0},{ 1, 0},{ 2, 0},
		{-2, 1},{-1, 1},{ 0, 1},{ 1, 1},{ 2, 1},
		{-2, 2},{-1, 2},{ 0, 2},{ 1, 2},{ 2, 2},
	};
#endif

	double rsum = 0;
	double gsum = 0;
	double bsum = 0;
	for (int i = 0; i < DITHER_KERNEL_SIZE2; ++i) {
		int xx = x + coords[i].x;
		int yy = y + coords[i].y;

		// mirror at the edges
		if (xx < 0) xx = -xx;
		else if (xx >= gen->getBlockWidth()*gen->getWidth())  xx = 2*(gen->getBlockWidth()*gen->getWidth()  - 1) - xx;
		if (yy < 0) yy = -yy;
		else if (yy >= 8*gen->getHeight()) yy = 2*(8*gen->getHeight() - 1) - yy;

		rsum += gen->pixelAt(xx, yy).r * eval_weights[i];
		gsum += gen->pixelAt(xx, yy).g * eval_weights[i];
		bsum += gen->pixelAt(xx, yy).b * eval_weights[i];
	}

	rsum /= eval_weightsum;
	gsum /= eval_weightsum;
	bsum /= eval_weightsum;

	if (rsum < 0) rsum = 0;
	else if (rsum > 1) rsum = 1;
	if (gsum < 0) gsum = 0;
	else if (gsum > 1) gsum = 1;
	if (bsum < 0) bsum = 0;
	else if (bsum > 1) bsum = 1;

	Color avgColor = {
		rsum,
		gsum,
		bsum,
		0,
	};

	return evaluate_color(srcColor, &avgColor);
}

// evaluate a pixel and its neighbors that are affected by it
double evaluate_pixel(const Block *src, const Block *gen, int x, int y)
{
#if 0
	return evaluate_single_pixel(src, gen, x, y);
#else
	double score = 0;
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			score += evaluate_single_pixel(src, gen, x+xx, y+yy);
		}
	}
	return score;
#endif
}

// evaluate a pixel and its neighbors that are affected by it
double evaluate_pixel(const Image *src, const Image *gen, int x, int y)
{
	double score = 0;
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			score += evaluate_single_pixel(src, gen, x+xx, y+yy);
		}
	}
	return score;
}

double evaluate(const Block *src, const Block *gen)
{
	double error = 0;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < src->getWidth(); ++x) {
			error += evaluate_pixel(src, gen, x, y);
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
