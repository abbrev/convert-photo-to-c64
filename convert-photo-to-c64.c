#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define HAVE_BOOL 1 // don't let pam.h define bool

#include <pam.h>

#include "evaluate.h"
#include "generate.h"

typedef struct {
	int width, height;
	Block *blocks;
} Image;

// X and Y are block coordinates
#define BLOCKAT(X, Y, IMAGEP) ((IMAGEP)->blocks[(Y)*(IMAGEP)->width + (X)])

// X and Y are pixel coordinates
#define PIXELAT(X, Y, IMAGEP) (BLOCKAT(X/8, Y/8, IMAGEP).pixels[Y%8][X%8])

static void load_image(FILE *pamfile, Image *image)
{
	image->width  = 0;
	image->height = 0;

	struct pam inpam;

	pnm_readpaminit(pamfile, &inpam, sizeof inpam);

	//fprintf(stderr, "width:  %d\n", inpam.width);
	//fprintf(stderr, "height: %d\n", inpam.height);
	//fprintf(stderr, "depth:  %u\n", inpam.depth);

	// Must be RGB
	if (inpam.depth != 1 && inpam.depth != 3) {
		fprintf(stderr, "input file must have a depth of 1 or 3!\n");
		return;
	}

	#if 0 // for development
	if (inpam.width != 8 || inpam.height != 8) {
		fprintf(stderr, "input file must have dimensions equal to 8!\n");
		return;
	}
	#endif

	// Support only a whole number of blocks in each dimension
	if ((inpam.width % 8) != 0 || (inpam.height % 8) != 0) {
		fprintf(stderr, "input file must have dimensions that are multiples of 8!\n");
		return;
	}

	// Enforce arbitrary size limits (maybe can lift this)
	if (inpam.width > 4000 || inpam.height > 4000) {
		fprintf(stderr, "input file must not be larger than 4000x4000!\n");
		return;
	}

	int size = inpam.width * inpam.height * sizeof *image->blocks;
	//fprintf(stderr, "size=%d\n", size);
	image->blocks = malloc(size);
	//fprintf(stderr, "blocks=%p\n", image->blocks);

	if (!image->blocks) {
		perror("malloc");
		return;
	}

	image->width  = inpam.width / 8;
	image->height = inpam.height / 8;

	tuple *tuplerow = pnm_allocpamrow(&inpam);
	//fprintf(stderr, "tuplerow=%p\n", tuplerow);
	//for (int x = 0; x < inpam.width; ++x) {
		//fprintf(stderr, "tuplerow[%d]=%p\n", x, tuplerow[x]);
	//}

	int rindex = 0;
	int gindex = 1;
	int bindex = 2;

	if (inpam.depth == 1) {
		rindex = gindex = bindex = 0;
	}

	for (int y = 0; y < inpam.height; ++y) {
		pnm_readpamrow(&inpam, tuplerow);
		for (int x = 0; x < inpam.width; ++x) {
			PIXELAT(x, y, image) = (Color){
				tuplerow[x][rindex] * 255 / inpam.maxval,
				tuplerow[x][gindex] * 255 / inpam.maxval,
				tuplerow[x][bindex] * 255 / inpam.maxval,
			};
		}
	}

	pnm_freepamrow(tuplerow);
}

static void save_image(FILE *pamfile, const Image *image)
{
	//fprintf(stderr, "%s(%p, %p)\n", __func__, pamfile, image);

#if 0
	for (int y = 0; y < image->height; ++y) {
		for (int x = 0; x < image->width; ++x) {
			for (int yy = 0; yy < 8; ++yy) {
				for (int xx = 0; xx < 8; ++xx) {
					fprintf(stderr, " #%02x%02x%02x",
					BLOCKAT(x, y, image).pixels[yy][xx].r,
					BLOCKAT(x, y, image).pixels[yy][xx].g,
					BLOCKAT(x, y, image).pixels[yy][xx].b);
				}
				fprintf(stderr, "\n");
			}
		}
	}
#endif

	struct pam outpam = {};

	outpam.size = sizeof outpam;
	outpam.len = sizeof outpam;
	outpam.file = pamfile;
	outpam.format = PPM_FORMAT;
	outpam.width = image->width * 8;
	outpam.height = image->height * 8;
	outpam.depth = 3;
	outpam.maxval = 255;
	outpam.bytes_per_sample = 1;
	outpam.plainformat = 1;
	strcpy(outpam.tuple_type, PAM_PPM_TUPLETYPE);

	pnm_writepaminit(&outpam);

	tuple *tuplerow = pnm_allocpamrow(&outpam);
	for (int y = 0; y < image->height * 8; ++y) {
		for (int x = 0; x < image->width * 8; ++x) {
			const Color *p = &PIXELAT(x, y, image);
			tuplerow[x][0] = outpam.maxval * p->r / 255;
			tuplerow[x][1] = outpam.maxval * p->g / 255;
			tuplerow[x][2] = outpam.maxval * p->b / 255;
		}
		pnm_writepamrow(&outpam, tuplerow);
	}

#if 0
	for (int y = 0; y < image->height; ++y) {
		for (int yy = 0; yy < 8; ++yy) {
			for (int x = 0; x < image->width; ++x) {
				const Block *b = &BLOCKAT(x, y, image);
				for (int xx = 0; xx < 8; ++xx) {
					const Color *p = &b->pixels[yy][xx];
					tuplerow[8*x+xx][0] = p->r;
					tuplerow[8*x+xx][1] = p->g;
					tuplerow[8*x+xx][2] = p->b;
				}
			}
			pnm_writepamrow(&outpam, tuplerow);
		}
	}
#endif
	pnm_freepamrow(tuplerow);
}

static double dither_weight = 1.0;

static bool process_opts(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "w:")) != -1) {
		switch (opt) {
		case 'w': {
				  double w = atof(optarg);
				  if (w < 0 || w > 1000) {
					  fprintf(stderr, "weight must be between 0%% and 1000%%\n");
					  return false;
				  }
				  dither_weight = w / 100.;
			  }
			  break;
		default:
			return false;
		}
	}

	return true;
}

#define GAMMA 2.2

int main(int argc, char *argv[])
{
	srand(time(NULL));
	pnm_init(&argc, argv);

	Image image;
	load_image(stdin, &image);

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
	if (!image.blocks) {
		//fprintf(stderr, "%s (%d): bad image?\n", __FILE__, __LINE__);
		return 1;
	}

#if 0
	// de-gamma the input image
	for (int y = 0; y < image.height*8; ++y) {
		for (int x = 0; x < image.width*8; ++x) {
			PIXELAT(x, y, &image).r = 255 * pow(PIXELAT(x, y, &image).r / 255.0, GAMMA);
			PIXELAT(x, y, &image).g = 255 * pow(PIXELAT(x, y, &image).g / 255.0, GAMMA);
			PIXELAT(x, y, &image).b = 255 * pow(PIXELAT(x, y, &image).b / 255.0, GAMMA);
		}
	}
#endif

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#if 0
	save_image(stdout, &image);
#else
	Image output = image;
	output.blocks = malloc(output.width * output.height * sizeof *output.blocks);

	#define NUM_COLORS 16
	Color all_colors[NUM_COLORS] = {
#if 0
		// pepto
		{  0,   0,   0},/* 0       black	*/
		{255, 255, 255},/* 1       white	*/
		{116,  67,  53},/* 2       red		*/
		{124, 172, 186},/* 3       cyan		*/
		{123,  72, 144},/* 4       purple 	*/
		{100, 151,  79},/* 5       green	*/
		{ 64,  50, 133},/* 6       blue		*/
		{191, 205, 122},/* 7       yellow	*/
		{123,  91,  47},/* 8       orange	*/
		{ 79,  69,   0},/* 9       brown	*/
		{163, 114, 101},/* A light red		*/
		{ 80,  80,  80},/* B  dark grey		*/
		{120, 120, 120},/* C       grey		*/
		{164, 215, 142},/* D light green	*/
		{120, 106, 189},/* E light blue		*/
		{159, 159, 159},/* F light grey		*/
#else
		// colodore
		{  0,   0,   0},/* 0       black	*/
		{255, 255, 255},/* 1       white	*/
		{129,  51,  56},/* 2       red		*/
		{117, 206, 200},/* 3       cyan		*/
		{142,  60, 151},/* 4       purple 	*/
		{ 86, 172,  77},/* 5       green	*/
		{ 46,  44, 155},/* 6       blue		*/
		{237, 241, 113},/* 7       yellow	*/
		{142,  80,  41},/* 8       orange	*/
		{ 85,  56,   0},/* 9       brown	*/
		{196, 108, 113},/* A light red		*/
		{ 74,  74,  74},/* B  dark grey		*/
		{123, 123, 123},/* C       grey		*/
		{169, 255, 159},/* D light green	*/
		{112, 109, 235},/* E light blue		*/
		{178, 178, 178},/* F light grey		*/
#endif
	};

	// Colors are gamma corrected. Make them linear.
	for (int i = 0; i < NUM_COLORS; ++i) {
		all_colors[i].r = 255 * pow(all_colors[i].r / 255.0, 2.2);
		all_colors[i].g = 255 * pow(all_colors[i].g / 255.0, 2.2);
		all_colors[i].b = 255 * pow(all_colors[i].b / 255.0, 2.2);
	}

	if (!process_opts(argc, argv)) {
		exit(1);
	}

	set_dither_weight(dither_weight);

	Color colors[2];

	for (int y = 0; y < output.height; ++y) {
		//fprintf(stderr, "\r%d%%", (100 * y + output.height / 2) / output.height);
		for (int x = 0; x < output.width; ++x) {
			fprintf(stderr, "\r%.2f%%", 100.0 * (y * output.width + x) / (output.height * output.width));
			double best_score = 9999999999;
			int best_color0ix = 0;
			int best_color1ix = 1;
			for (int color0ix = 0; color0ix < NUM_COLORS - 1; ++color0ix) {
				for (int color1ix = color0ix + 1; color1ix < NUM_COLORS; ++color1ix) {
					colors[0] = all_colors[color0ix];
					colors[1] = all_colors[color1ix];
					
					generate(&BLOCKAT(x, y, &image), colors, &BLOCKAT(x, y, &output));
					double score = evaluate(&BLOCKAT(x, y, &image), &BLOCKAT(x, y, &output));
					if (score < best_score) {
						best_score = score;
						best_color0ix = color0ix;
						best_color1ix = color1ix;
						if (score == 0) {
							goto perfect;
						}
					}
				}
			}
perfect:
			colors[0] = all_colors[best_color0ix];
			colors[1] = all_colors[best_color1ix];
			generate(&BLOCKAT(x, y, &image), colors, &BLOCKAT(x, y, &output));
		}
	}
	fprintf(stderr, "\r%.2f%%", 100.0);
	fprintf(stderr, "\n");

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);

	// gamma the output image
	for (int y = 0; y < output.height*8; ++y) {
		for (int x = 0; x < output.width*8; ++x) {
			PIXELAT(x, y, &output).r = 255 * pow(PIXELAT(x, y, &output).r / 255.0, 1.0/GAMMA);
			PIXELAT(x, y, &output).g = 255 * pow(PIXELAT(x, y, &output).g / 255.0, 1.0/GAMMA);
			PIXELAT(x, y, &output).b = 255 * pow(PIXELAT(x, y, &output).b / 255.0, 1.0/GAMMA);
		}
	}

	save_image(stdout, &output);
	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#endif

	return 0;
}
