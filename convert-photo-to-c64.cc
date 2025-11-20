#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define HAVE_BOOL 1 // don't let pam.h define bool

extern "C" {
#include <pam.h>
}

#include "image.h"
#include "evaluate.h"
#include "generate.h"

static double from_gamma(double d)
{
	return d <= 0.04045 ? d / 12.92 : pow((d+0.055)/1.055, 2.4);
}

static double to_gamma(double d)
{
	return d <= 0.0031308 ? 12.92*d : pow(1.055*d, 1/2.4) - 0.055;
}


static Image *load_image(FILE *pamfile)
{
	Image *image = nullptr;

	struct pam inpam;

	pnm_readpaminit(pamfile, &inpam, sizeof inpam);

	//fprintf(stderr, "width:  %d\n", inpam.width);
	//fprintf(stderr, "height: %d\n", inpam.height);
	//fprintf(stderr, "depth:  %u\n", inpam.depth);

	// Must be RGB
	if (inpam.depth != 1 && inpam.depth != 3) {
		fprintf(stderr, "input file must have a depth of 1 or 3!\n");
		return nullptr;
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
		return nullptr;
	}

	// Enforce arbitrary size limits (maybe can lift this)
	if (inpam.width > 4000 || inpam.height > 4000) {
		fprintf(stderr, "input file must not be larger than 4000x4000!\n");
		return nullptr;
	}

	image = new Image(inpam.width / 8, inpam.height / 8);

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
			image->pixelAt(x, y) = (Color){
				from_gamma((double)tuplerow[x][rindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][gindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][bindex] / inpam.maxval),
			};
		}
	}

	pnm_freepamrow(tuplerow);

	return image;
}

static void save_image(FILE *pamfile, const Image *image)
{
	//fprintf(stderr, "%s(%p, %p)\n", __func__, pamfile, image);

#if 0
	for (int y = 0; y < image->getHeight(); ++y) {
		for (int x = 0; x < image->getWidth(); ++x) {
			for (int yy = 0; yy < 8; ++yy) {
				for (int xx = 0; xx < 8; ++xx) {
					fprintf(stderr, " #%02x%02x%02x",
					image->blockAt(x, y).pixels[yy][xx].r,
					image->blockAt(x, y).pixels[yy][xx].g,
					image->blockAt(x, y).pixels[yy][xx].b);
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
	outpam.width = image->getWidth() * 8;
	outpam.height = image->getHeight() * 8;
	outpam.depth = 3;
	outpam.maxval = 255;
	outpam.bytes_per_sample = 1;
	outpam.plainformat = 1;
	strcpy(outpam.tuple_type, PAM_PPM_TUPLETYPE);

	pnm_writepaminit(&outpam);

	int rindex = 0;
	int gindex = 1;
	int bindex = 2;

	tuple *tuplerow = pnm_allocpamrow(&outpam);
	for (int y = 0; y < image->getHeight() * 8; ++y) {
		for (int x = 0; x < image->getWidth() * 8; ++x) {
			const Color& p = image->pixelAt(x, y);
			tuplerow[x][rindex] = round(outpam.maxval * p.r);
			tuplerow[x][gindex] = round(outpam.maxval * p.g);
			tuplerow[x][bindex] = round(outpam.maxval * p.b);
		}
		pnm_writepamrow(&outpam, tuplerow);
	}

#if 0
	for (int y = 0; y < image->getHeight(); ++y) {
		for (int yy = 0; yy < 8; ++yy) {
			for (int x = 0; x < image->getWidth(); ++x) {
				const Block *b = &image->blockAt(x, y);
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

static double dither_weight = 4.0;

static bool process_opts(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "d:")) != -1) {
		switch (opt) {
		case 'd': {
				  double d = atof(optarg);
				  if (d < 0) {
					  fprintf(stderr, "Error: dither strength must not be less than 0\n");
					  return false;
				  }
				  dither_weight = d / 100.;
			  }
			  break;
		default:
			return false;
		}
	}

	return true;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	pnm_init(&argc, argv);

	Image *image = load_image(stdin);

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
	if (!image) {
		//fprintf(stderr, "%s (%d): bad image?\n", __FILE__, __LINE__);
		return 1;
	}

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#if 0
	save_image(stdout, &image);
#else
	Image output = *image;

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
		all_colors[i].r = from_gamma(all_colors[i].r / 255.0);
		all_colors[i].g = from_gamma(all_colors[i].g / 255.0);
		all_colors[i].b = from_gamma(all_colors[i].b / 255.0);
	}

	if (!process_opts(argc, argv)) {
		exit(1);
	}

	set_dither_weight(dither_weight);

#define NUM_COLORS_PER_BLOCK 2
	Color colors[NUM_COLORS_PER_BLOCK];

#define BLOCK_PASSES 1
	for (int y = 0; y < output.getHeight(); ++y) {
		//fprintf(stderr, "\r%d%%", (100 * y + output.getHeight() / 2) / output.getHeight());
		for (int x = 0; x < output.getWidth(); ++x) {
			fprintf(stderr, "\rgenerating %.2f%%", 100.0 * (y * output.getWidth() + x) / (output.getHeight() * output.getWidth()));
			double best_score = INFINITY;
			int best_color_ix[2] = { 0, 1 };
			Block& input_block = image->blockAt(x, y);
			Block& output_block = output.blockAt(x, y);
			for (int color0ix = 0; color0ix < NUM_COLORS - 1; ++color0ix) {
				colors[0] = all_colors[color0ix];
				input_block.setPalette(0, colors[0]);
				for (int color1ix = color0ix + 1; color1ix < NUM_COLORS; ++color1ix) {
					colors[1] = all_colors[color1ix];
					input_block.setPalette(1, colors[1]);
					
					generate(&input_block, colors, &output_block);
					refine_genetically(&input_block, &output_block, BLOCK_PASSES);

					double score = evaluate(&input_block, &output_block);
					if (score < best_score) {
						best_score = score;
						best_color_ix[0] = color0ix;
						best_color_ix[1] = color1ix;
						if (score == 0) {
							goto perfect;
						}
					}
				}
			}
perfect:
			//fprintf(stderr, "best colors for (%2d, %2d): %2d %2d\n", x, y, best_color_ix[0], best_color_ix[1]);

			for (int c = 0; c < NUM_COLORS_PER_BLOCK; ++c) {
				colors[c] = all_colors[best_color_ix[c]];
				input_block.setPalette(c, all_colors[best_color_ix[c]]);
			}
			generate(&input_block, colors, &output_block);
		}
	}

#if 1
	// refine
			//fprintf(stderr, "\rrefining   %.2f%%", 100.0 * (y * 8 + x) / (8 * 8));
	refine_genetically(image, &output);
#endif

	fprintf(stderr, "\r%.2f%%", 100.0);
	fprintf(stderr, "\n");

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);

	// gamma the output image
	for (int y = 0; y < output.getHeight()*8; ++y) {
		for (int x = 0; x < output.getWidth()*8; ++x) {
			output.pixelAt(x, y).r = to_gamma(output.pixelAt(x, y).r);
			output.pixelAt(x, y).g = to_gamma(output.pixelAt(x, y).g);
			output.pixelAt(x, y).b = to_gamma(output.pixelAt(x, y).b);
		}
	}

	save_image(stdout, &output);
	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#endif

	return 0;
}
