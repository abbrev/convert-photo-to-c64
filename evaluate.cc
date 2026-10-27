#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "evaluate.h"

// see http://www-labs.iro.umontreal.ca/~mignotte/IFT2425/Documents/EfficientApproximationArctgFunction.pdf
//#define ATAN(x) ((M_PI/4) * (x) + 0.273*(x)*(1 - abs((x))))
#define ATAN(x) (0.273*(x)*(((M_PI/4/0.273)+1) - abs((x)))) // refactored

static double my_atan2(double q, double i)
{
	double r;
	if (abs(q) > abs(i)) {
		r = M_PI/2 - ATAN(abs(i) / abs(q));         // arctan(z) = 90-arctan(1/z)
	} else {
		r = (i == 0) ? 0 : ATAN(abs(q) / abs(i));   // arctan(z)
	}
	r = (i < 0) ? M_PI - r : r;                         // arctan(-z) = -arctan(z)
	return (q < 0) ? -r : r;                            // arctan(-z) = -arctan(z)
}

double evaluate_color2(const Color& src, const Color& gen)
{
#if 1
#define R_WEIGHT (0.299)
#define G_WEIGHT (0.587)
#define B_WEIGHT (0.114)
#else
#define R_WEIGHT (1./3)
#define G_WEIGHT (1./3)
#define B_WEIGHT (1./3)
#endif

	return pow(R_WEIGHT * pow(abs(src.r() - gen.r()), 2.0) +
		   G_WEIGHT * pow(abs(src.g() - gen.g()), 2.0) +
		   B_WEIGHT * pow(abs(src.b() - gen.b()), 2.0), 0.5);
}

double y_weight = 4.0;

double evaluate_color(const Color& src, const Color& gen)
{
#if 1
	double src_ydbdr_y = src.ydbdr_y();
	double gen_ydbdr_y = gen.ydbdr_y();
	double  y_diff = (src_ydbdr_y  - gen_ydbdr_y) * (y_weight*1.333);

	double src_ydbdr_db = src.ydbdr_db();
	double src_ydbdr_dr = src.ydbdr_dr();
	double gen_ydbdr_db = gen.ydbdr_db();
	double gen_ydbdr_dr = gen.ydbdr_dr();

#if 0
	double src_theta = my_atan2(src_ydbdr_db, src_ydbdr_dr);
	double gen_theta = my_atan2(gen_ydbdr_db, gen_ydbdr_dr);
	double theta_diff = abs(src_theta - gen_theta);
	if (theta_diff < 0) {
		theta_diff += 2*M_PI;
	} else if (theta_diff > 2*M_PI) {
		theta_diff -= 2*M_PI;
	}

	double arc_factor = 1 + theta_diff - theta_diff/M_PI;
#else
	double arc_factor = 1;
#endif
#if 0
	fprintf(stderr, "src=(%+5.3f, %+5.3f):%+5.3f gen=(%+5.3f, %+5.3f):%+5.3f diff=%+5.3f arc_factor=%+5.3f\n",
	        src.ydbdr_db(), src.ydbdr_dr(), src_theta,
		gen.ydbdr_db(), gen.ydbdr_dr(), gen_theta,
		theta_diff,
		arc_factor);
#endif

	double db_diff = (src_ydbdr_db - gen_ydbdr_db) * arc_factor;
	double dr_diff = (src_ydbdr_dr - gen_ydbdr_dr) * arc_factor;

	return sqrt(( y_diff *  y_diff) +
	            (db_diff * db_diff) +
	            (dr_diff * dr_diff));
#elif 1
	double src_yuv_y = src.yuv_y();
	double gen_yuv_y = gen.yuv_y();
	double y_diff = (src_yuv_y  - gen_yuv_y) * (y_weight*0.5255);

	double src_yuv_u = src.yuv_u();
	double src_yuv_v = src.yuv_v();
	double gen_yuv_u = gen.yuv_u();
	double gen_yuv_v = gen.yuv_v();

	double u_diff = (src_yuv_u - gen_yuv_u);
	double v_diff = (src_yuv_v - gen_yuv_v);

	return ( y_diff *  y_diff) +
	       ((u_diff * u_diff) + (v_diff * v_diff));
#elif 1
	double src_ycbcr_y = src.ycbcr_y();
	double gen_ycbcr_y = gen.ycbcr_y();
	double y_diff = (src_ycbcr_y  - gen_ycbcr_y) * (y_weight*0.5);

	double src_ycbcr_cb = src.ycbcr_cb();
	double src_ycbcr_cr = src.ycbcr_cr();
	double gen_ycbcr_cb = gen.ycbcr_cb();
	double gen_ycbcr_cr = gen.ycbcr_cr();

	double cb_diff = (src_ycbcr_cb - gen_ycbcr_cb);
	double cr_diff = (src_ycbcr_cr - gen_ycbcr_cr);

	return ( y_diff *  y_diff) +
	       ((cb_diff * cb_diff) + (cr_diff * cr_diff));
#elif 1
#define L_POWER  3.0
#define L_WEIGHT 1.0 // TODO adjust as needed
	double l_diff = (pow(src.oklab_l(), L_POWER) - pow(gen.oklab_l(), L_POWER)) * L_WEIGHT;
	double a_diff = (src.oklab_a() - gen.oklab_a());
	double b_diff = (src.oklab_b() - gen.oklab_b());

	return (l_diff * l_diff) +
	       (a_diff * a_diff) + (b_diff * b_diff);
#elif 1
#define I_WEIGHT 3.0 // TODO adjust as needed
	double i_diff = (src.i() - gen.i()) * I_WEIGHT;
	double p_diff = (src.p() - gen.p());
	double t_diff = (src.t() - gen.t());

	return (i_diff * i_diff) +
	       (p_diff * p_diff) + (t_diff * t_diff);
#else
		return ((src.r() - gen.r()) * (src.r() - gen.r())*(R_WEIGHT) +
		        (src.g() - gen.g()) * (src.g() - gen.g())*(G_WEIGHT) +
		        (src.b() - gen.b()) * (src.b() - gen.b())*(B_WEIGHT));
#endif
}

#define DITHER_KERNEL_SIZE2 (DITHER_KERNEL_SIZE*DITHER_KERNEL_SIZE)

#if 1
#define DITHER_KERNEL_SIZE 3
static const int dither_pattern[DITHER_KERNEL_SIZE2] = {
	1, 8, 1,
	4, 0, 4,
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

// evaluate a single pixel
double evaluate_single_pixel(const Block *src, const Block *gen, int x, int y)
{
	if (x < 0 || src->getWidth() <= x ||
	    y < 0 || 8 <= y) return 0;

	const Color& srcColor = src->pixelAt(x, y);

	// compute weighted average color using neighboring pixels

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

		const Color& gen_pixel = gen->pixelAt(xx, yy);
		rsum += gen_pixel.r() * eval_weights[i];
		gsum += gen_pixel.g() * eval_weights[i];
		bsum += gen_pixel.b() * eval_weights[i];
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

	Color avgColor = Color(
		rsum,
		gsum,
		bsum,
		0
	);

	return evaluate_color(srcColor, avgColor);
}

// evaluate a single pixel
double evaluate_single_pixel(const Image *src, const Image *gen, int x, int y)
{
	if (x < 0 || gen->getBlockWidth()*gen->getWidth()  <= x ||
	    y < 0 || 8*gen->getHeight() <= y) return 0;

	const Color& srcColor = src->pixelAt(x, y);

	// compute weighted average color using neighboring pixels

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

		const Color& gen_pixel = gen->pixelAt(xx, yy);
		rsum += gen_pixel.r() * eval_weights[i];
		gsum += gen_pixel.g() * eval_weights[i];
		bsum += gen_pixel.b() * eval_weights[i];
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

	Color avgColor = Color(
		rsum,
		gsum,
		bsum,
		0
	);

	return evaluate_color(srcColor, avgColor);
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
