#include <errno.h>
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
	return pow(d, 2.2);
}

static double to_gamma(double d)
{
	return pow(d, 1/2.2);
}


enum GfxMode {
	HIRES,
	MULTICOLOR,
};

#define NUM_COLORS 16

static GfxMode gfx_mode = MULTICOLOR;
static int bgcolorix = 0xB;
static int num_palette_colors = NUM_COLORS;

struct Palette {
	const char *name;
	Color colors[NUM_COLORS];
};

static Palette custom_palette;

static bool load_palette(const char *filename)
{
	bool ret = false;
	char line[9001];

	FILE *f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "Error: cannot open \"%s\": %s\n", filename, strerror(errno));
		return false;
	}

	int c = 0;
	for (int lineno = 1; fgets(line, sizeof line, f); ++lineno) {
		int r, g, b;
		if (line[0] == '#' || line[0] == '\n') {
			continue;
		}
		if (sscanf(line, "%x %x %x", &r, &g, &b) != 3) {
			fprintf(stderr, "Error: malformed palette file at line %d\n", lineno);
			goto out;
		}
		if (c >= NUM_COLORS) {
			fprintf(stderr, "Error: too many colors\n");
			goto out;
		}
		custom_palette.colors[c] = { (double)r, (double)g, (double)b, c };
		++c;
	}
	num_palette_colors = c;
	ret = true;

out:
	fclose(f);
	return ret;
}

static Image *load_image(FILE *pamfile, int block_width)
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
	if ((inpam.width % block_width) != 0 || (inpam.height % 8) != 0) {
		fprintf(stderr, "input file must have dimensions that are multiples of 8!\n");
		return nullptr;
	}

	// Enforce arbitrary size limits (maybe can lift this)
	if (inpam.width > 4000 || inpam.height > 4000) {
		fprintf(stderr, "input file must not be larger than 4000x4000!\n");
		return nullptr;
	}

	image = new Image(inpam.width / block_width, inpam.height / 8);

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

	image->setBlockWidth(block_width);

	for (int y = 0; y < inpam.height; ++y) {
		pnm_readpamrow(&inpam, tuplerow);
		for (int x = 0; x < inpam.width; ++x) {
			image->pixelAt(x, y) = {
				from_gamma((double)tuplerow[x][rindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][gindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][bindex] / inpam.maxval),
				0,
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
	outpam.width = image->getWidth() * image->getBlockWidth();
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
		for (int x = 0; x < image->getWidth() * image->getBlockWidth(); ++x) {
			const Color& p = image->pixelAt(x, y);
			tuplerow[x][rindex] = round(outpam.maxval * p.r);
			tuplerow[x][gindex] = round(outpam.maxval * p.g);
			tuplerow[x][bindex] = round(outpam.maxval * p.b);
		}
		pnm_writepamrow(&outpam, tuplerow);
	}

	pnm_freepamrow(tuplerow);
}

static Palette palettes[] = {
	{
		"colodore",
		{
			{  0,   0,   0, 0x0},/* 0       black    */
			{255, 255, 255, 0x1},/* 1       white    */
			{129,  51,  56, 0x2},/* 2       red      */
			{117, 206, 200, 0x3},/* 3       cyan     */
			{142,  60, 151, 0x4},/* 4       purple   */
			{ 86, 172,  77, 0x5},/* 5       green    */
			{ 46,  44, 155, 0x6},/* 6       blue     */
			{237, 241, 113, 0x7},/* 7       yellow   */
			{142,  80,  41, 0x8},/* 8       orange   */
			{ 85,  56,   0, 0x9},/* 9       brown    */
			{196, 108, 113, 0xA},/* A light red      */
			{ 74,  74,  74, 0xB},/* B  dark grey     */
			{123, 123, 123, 0xC},/* C       grey     */
			{169, 255, 159, 0xD},/* D light green    */
			{112, 109, 235, 0xE},/* E light blue     */
			{178, 178, 178, 0xF},/* F light grey     */
		}
	},
	{
		"pepto",
		{
			{  0,   0,   0, 0x0},/* 0       black    */
			{255, 255, 255, 0x1},/* 1       white    */
			{116,  67,  53, 0x2},/* 2       red      */
			{124, 172, 186, 0x3},/* 3       cyan     */
			{123,  72, 144, 0x4},/* 4       purple   */
			{100, 151,  79, 0x5},/* 5       green    */
			{ 64,  50, 133, 0x6},/* 6       blue     */
			{191, 205, 122, 0x7},/* 7       yellow   */
			{123,  91,  47, 0x8},/* 8       orange   */
			{ 79,  69,   0, 0x9},/* 9       brown    */
			{163, 114, 101, 0xA},/* A light red      */
			{ 80,  80,  80, 0xB},/* B  dark grey     */
			{120, 120, 120, 0xC},/* C       grey     */
			{164, 215, 142, 0xD},/* D light green    */
			{120, 106, 189, 0xE},/* E light blue     */
			{159, 159, 159, 0xF},/* F light grey     */
		}
	},
};

static Palette *palette = &palettes[0];
static double dither_weight = 4.0;
static double saturation = 1.0;

static bool process_opts(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "b:d:m:p:s:P:")) != -1) {
		switch (opt) {
			case 'b': {
				int b = atoi(optarg);
				bgcolorix = b;
				break;
			}
			case 'd': {
				double d = atof(optarg);
				if (d < 0) {
					fprintf(stderr, "Error: dither strength must not be less than 0\n");
					return false;
				}
				dither_weight = d / 100.;
				break;
			}
			case 'm': {
				const char *mode = optarg;
				if (strcasecmp(mode, "hires") == 0) {
					gfx_mode = HIRES;
				} else if (strcasecmp(mode, "multicolor") == 0) {
					gfx_mode = MULTICOLOR;
				} else {
					fprintf(stderr, "Error: unknown mode \"%s\"\n", mode);
					return false;
				}
				break;
			}
			case 'p': {
				const char *name = optarg;
				palette = nullptr;
				for (size_t i = 0; i < sizeof palettes/sizeof *palettes; ++i) {
					Palette& p = palettes[i];
					if (strcasecmp(p.name, name) == 0) {
						palette = &p;
						break;
					}
				}
				if (!palette) {
					fprintf(stderr, "Error: unknown palette \"%s\"\n", name);
					return false;
				}
				break;
			}
			#if 1
			case 'P': {
				const char *filename = optarg;
				// TODO read palette from file
				if (!load_palette(filename)) {
					return false;
				}
				palette = &custom_palette;
				break;
			}
			#endif
			case 's': {
				double s = atof(optarg);
				saturation = s / 100.;
				break;
			}
			default:
				return false;
		}
	}

	return true;
}

int main(int argc, char *argv[])
{
	if (!process_opts(argc, argv)) {
		exit(1);
	}

	if (bgcolorix < 0 || num_palette_colors <= bgcolorix) {
		fprintf(stderr, "Error: bg color out of bounds\n");
		return 1;
	}

	srand(time(NULL));
	pnm_init(&argc, argv);

	int block_width;
	int color0max;
	int color1max;
	int num_colors_per_block;

	if (gfx_mode == HIRES) {
		color0max = num_palette_colors - 1;
		color1max = num_palette_colors - 0;
		num_colors_per_block = 2;
		block_width = 8;
	} else if (gfx_mode == MULTICOLOR) {
		color0max = num_palette_colors - 2;
		color1max = num_palette_colors - 1;
		num_colors_per_block = 4;
		block_width = 4;
	} else {
		fprintf(stderr, "Error: programming error!\n");
		return 1;
	}

	Image *image = load_image(stdin, block_width);

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
	if (!image) {
		//fprintf(stderr, "%s (%d): bad image?\n", __FILE__, __LINE__);
		return 1;
	}

	for (int y = 0; y < 8*image->getHeight(); ++y) {
		for (int x = 0; x < image->getBlockWidth()*image->getWidth(); ++x) {
			Color& c = image->pixelAt(x, y);
			double y = 0.2126*c.r + 0.7152*c.g + 0.0722*c.b;
			double dr = c.r - y;
			double dg = c.g - y;
			double db = c.b - y;
			c.r = y + saturation * dr;
			c.g = y + saturation * dg;
			c.b = y + saturation * db;
		}
	}

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#if 0
	save_image(stdout, &image);
#else
	Image output = *image;

	// Palette colors are gamma corrected. Make them linear.
	for (int i = 0; i < NUM_COLORS; ++i) {
		palette->colors[i].r = from_gamma(palette->colors[i].r / 255.0);
		palette->colors[i].g = from_gamma(palette->colors[i].g / 255.0);
		palette->colors[i].b = from_gamma(palette->colors[i].b / 255.0);
	}

	set_dither_weight(dither_weight);

	Color bgcolor = palette->colors[bgcolorix];

	image->setBlockWidth(block_width);
	output.setBlockWidth(block_width);

#define BLOCK_PASSES 2
	for (int y = 0; y < output.getHeight(); ++y) {
		//fprintf(stderr, "\r%d%%", (100 * y + output.getHeight() / 2) / output.getHeight());
		for (int x = 0; x < output.getWidth(); ++x) {
			fprintf(stderr, "\rgenerating %.2f%%", 100.0 * (y * output.getWidth() + x) / (output.getHeight() * output.getWidth()));
			double best_score = INFINITY;
			int best_color_ix[4] = { 0, 1, 2, bgcolorix };
			Block& input_block = image->blockAt(x, y);
			Block& output_block = output.blockAt(x, y);

			input_block.setPaletteSize(num_colors_per_block);
			input_block.setWidth(block_width);
			output_block.setWidth(block_width);

			// XXX in multicolor mode, bit pattern 0b00 is used for the background color
			input_block.setPalette(3, bgcolor);
			output_block.setPalette(3, bgcolor);

			for (int color0ix = 0; color0ix < color0max; ++color0ix) {
				input_block.setPalette(0, palette->colors[color0ix]);
				output_block.setPalette(0, palette->colors[color0ix]);

				for (int color1ix = color0ix + 1; color1ix < color1max; ++color1ix) {
					input_block.setPalette(1, palette->colors[color1ix]);
					output_block.setPalette(1, palette->colors[color1ix]);

					int color2ix = color1ix + 1;
					if (gfx_mode == HIRES) {
						color2ix = num_palette_colors - 1;
					}

					for (; color2ix < num_palette_colors; ++color2ix) {
						input_block.setPalette(2, palette->colors[color2ix]);
						output_block.setPalette(2, palette->colors[color2ix]);

						generate(&input_block, &output_block);
						refine_genetically(&input_block, &output_block, BLOCK_PASSES);

						double score = evaluate(&input_block, &output_block);
						if (score < best_score) {
							best_score = score;
							best_color_ix[0] = color0ix;
							best_color_ix[1] = color1ix;
							best_color_ix[2] = color2ix;
							if (score == 0) {
								goto perfect;
							}
						}
					}
				}
			}
perfect:
			//fprintf(stderr, "best colors for (%2d, %2d): %2d %2d\n", x, y, best_color_ix[0], best_color_ix[1]);

			for (int c = 0; c < num_colors_per_block; ++c) {
				input_block.setPalette(c, palette->colors[best_color_ix[c]]);
			}
			generate(&input_block, &output_block);
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
		for (int x = 0; x < output.getWidth()*output.getBlockWidth(); ++x) {
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
