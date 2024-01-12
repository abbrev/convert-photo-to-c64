#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "evaluate.h"

// gamma-corrected values (for sRGB) (e.g., 128 => 0.21586, 188 => 0.50289, 255 => 1.00000 etc)
static double srgb[256];

// linear RGB
static double rgb[256];

double evaluate_color(const Color *src, const Color *gen)
{
	return ((srgb[src->r] - rgb[gen->r]) * (srgb[src->r] - rgb[gen->r]) +
	        (srgb[src->g] - rgb[gen->g]) * (srgb[src->g] - rgb[gen->g]) +
	        (srgb[src->b] - rgb[gen->b]) * (srgb[src->b] - rgb[gen->b]));
}

static const int dither_pattern[9] = {
	0, 1, 0,
	1, 0, 1,
	0, 1, 0,
};

#define CENTER_WEIGHT 10000 // some large value so small dither weights can be represented
static int eval_weights[9] = {
	0,0,0,
	0,1,0,
	0,0,0,
};
static int eval_weightsum = 1;

void set_dither_weight(double w)
{
	int total_dither_weight = 0;
	for (int i = 0; i < 9; ++i) {
		total_dither_weight += dither_pattern[i];
	}
	eval_weightsum = 0;
	for (int i = 0; i < 9; ++i) {
		eval_weights[i] = (i == 4) ? CENTER_WEIGHT : dither_pattern[i] * CENTER_WEIGHT * w / total_dither_weight;
		eval_weightsum += eval_weights[i];
	}
}

// evaluate a single pixel
double evaluate_single_pixel(const Block *src, const Block *gen, int x, int y)
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

		rsum += (int)gen->pixels[yy][xx].r * eval_weights[i];
		gsum += (int)gen->pixels[yy][xx].g * eval_weights[i];
		bsum += (int)gen->pixels[yy][xx].b * eval_weights[i];
	}

	Color avgColor = {
		rsum / eval_weightsum,
		gsum / eval_weightsum,
		bsum / eval_weightsum,
	};

	return evaluate_color(srcColor, &avgColor);
}

// evaluate a pixel and its neighbors that are affected by it
// apparently this doesn't work very well....
double evaluate_pixel(const Block *src, const Block *gen, int x, int y)
{
	double score = 0;
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			score += evaluate_single_pixel(src, gen, x+xx, y+yy);
		}
	}
	return score;
}

static void init_srgb(void)
{
        for (int i = 0; i < 256; ++i) {
                double u = (double)i/255;
                rgb[i] = u;
                srgb[i] = u <= 0.04045 ? u / 12.92 : pow((u+0.055)/1.055, 2.4); // srgb
                //srgb[i] = u <= 0.0031308 ? 12.92*u : pow(1.055*u, 1/2.4) - 0.055; // unsrgb (wrong way)
        }
}

double evaluate(const Block *src, const Block *gen)
{
	static bool srgb_is_inited = false;
	if (!srgb_is_inited) {
		init_srgb();
		srgb_is_inited = true;
	}
	double error = 0;
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
