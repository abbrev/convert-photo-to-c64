#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <algorithm>

#define HAVE_BOOL 1 // don't let pam.h define bool

extern "C" {
#include <pam.h>
}

#include "image.h"
#include "evaluate.h"
#include "generate.h"

#define GAMMA 2.2

static double from_gamma(double d)
{
	return pow(d, GAMMA);
}

static double to_gamma(double d)
{
	return pow(d, 1/GAMMA);
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
		unsigned r, g, b;
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
			image->pixelAt(x, y) = Color(
				from_gamma((double)tuplerow[x][rindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][gindex] / inpam.maxval),
				from_gamma((double)tuplerow[x][bindex] / inpam.maxval),
				0
			);
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
					image->blockAt(x, y).pixels[yy][xx].r(),
					image->blockAt(x, y).pixels[yy][xx].g(),
					image->blockAt(x, y).pixels[yy][xx].b());
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
			tuplerow[x][rindex] = round(outpam.maxval * to_gamma(p.r()));
			tuplerow[x][gindex] = round(outpam.maxval * to_gamma(p.g()));
			tuplerow[x][bindex] = round(outpam.maxval * to_gamma(p.b()));
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
static bool exclusions[NUM_COLORS]; // colors to exclude

static bool *get_colors_from_list(const char *s)
{
	static bool colors[NUM_COLORS];
	for (int i = 0; i < NUM_COLORS; ++i) {
		colors[i] = false;
	}
	char *endptr;
	for (const char *p = s; p[0] != '\0'; p = endptr) {
		long c = strtol(p, &endptr, 10);
		if (endptr > p) {
			if (0 <= c && c < NUM_COLORS) {
				colors[c] = true;
			} else {
				return nullptr;
			}
		}
	}
	return colors;
}

static const char *argv0;

static void usage(void)
{
	fprintf(stderr,
	        "Usage: %s [OPTIONS] <INPUT.pnm >OUTPUT.pnm\n"
	        "\n"
	        "Options:\n"
	        "  -b COLOR       Set background color.\n"
	        "  -d STRENGTH    Set dither strength (e.g. 400).\n"
	        "  -m MODE        Set output mode (\"hires\" or \"multicolor\").\n"
	        "  -p PALETTE     Load a palette (\"colodore\" or \"pepto\").\n"
	        "  -P FILE        Load palette from a VICE palette file.\n"
	        "  -i COLORLIST   Include colors (e.g., \"0 1 5\").\n"
	        "  -x COLORLIST   Exclude colors.\n"
	        "  -s SATURATION  Set saturation (0%% = grayscale, 100%% = full saturation).\n"
	        "  -y WEIGHT      Set Y weight (default %.1f)\n"
	        "  -h             Show this help page and exit.\n",
	        argv0, y_weight);
}

static bool process_opts(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "hb:d:i:x:m:p:s:P:y:")) != -1) {
		switch (opt) {
			case 'h': {
				usage();
				exit(0);
			}
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
			case 'i': {
				// include colors
				bool *colors = get_colors_from_list(optarg);
				if (!colors) {
					fprintf(stderr, "Error: bad color list format \"%s\"\n", optarg);
					return false;
				}
				for (int i = 0; i < NUM_COLORS; ++i) {
					exclusions[i] = !colors[i];
				}
				break;
			}
			case 'x': {
				// exclude colors
				bool *colors = get_colors_from_list(optarg);
				if (!colors) {
					fprintf(stderr, "Error: bad color list format \"%s\"\n", optarg);
					return false;
				}
				for (int i = 0; i < NUM_COLORS; ++i) {
					exclusions[i] = colors[i];
				}
				break;
			}
			case 's': {
				double s = atof(optarg);
				saturation = s / 100.;
				break;
			}
			case 'y': {
				y_weight = atof(optarg);
				break;
			}
			default:
				return false;
		}
	}

	return true;
}

static double calc_color_goodness(const Color& src, const Color& gen)
{
	static double highest = 0.0;
	double x = evaluate_color2(src, gen);
	if (x <= 0) {
		return 1.0;
	}
	if (x > highest) {
		highest = x;
		fprintf(stderr, "calc_color_goodness: highest=%4.1f\n", highest);
	}
#define MAX_X 1
	if (x > MAX_X) {
		x = MAX_X;
	}
	return pow(1.0 - x / MAX_X, 5.0);
}

int main(int argc, char *argv[])
{
	argv0 = argv[0];

	if (!process_opts(argc, argv)) {
		usage();
		exit(1);
	}

	if (bgcolorix < 0 || num_palette_colors <= bgcolorix) {
		fprintf(stderr, "Error: bg color out of bounds\n");
		return 1;
	}

	if (gfx_mode == MULTICOLOR && exclusions[bgcolorix]) {
		fprintf(stderr, "Error: bgcolor is excluded!\n");
		return 1;
	}

	srand(time(NULL));
	pnm_init(&argc, argv);

	int block_width;
	int color0max;
	int color1max;
	int num_colors_per_block;
	int num_global_colors = 0;

	if (gfx_mode == HIRES) {
		color0max = num_palette_colors - 0;
		color1max = num_palette_colors - 0;
		num_colors_per_block = 2;
		block_width = 8;
	} else if (gfx_mode == MULTICOLOR) {
		color0max = num_palette_colors - 0;
		color1max = num_palette_colors - 0;
		num_colors_per_block = 3;
		num_global_colors = 1;
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
			double y = 0.2126*c.r() + 0.7152*c.g() + 0.0722*c.b();
			double dr = c.r() - y;
			double dg = c.g() - y;
			double db = c.b() - y;
			c = Color(y + saturation * dr, y + saturation * dg, y + saturation * db, -1);
		}
	}

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#if 0
	save_image(stdout, &image);
#else
	Image output = *image;

	// Palette colors are gamma corrected. Make them linear.
	for (int i = 0; i < NUM_COLORS; ++i) {
		Color& c = palette->colors[i];
		c = Color(from_gamma(c.r() / 255.0), from_gamma(c.g() / 255.0), from_gamma(c.b() / 255.0), c.ix());
	}

	set_dither_weight(dither_weight);

	Color bgcolor = palette->colors[bgcolorix];

	// sort the palette by luminance
	//std::sort(palette->colors, palette->colors + num_palette_colors);

	image->setBlockWidth(block_width);
	output.setBlockWidth(block_width);

#define BLOCK_PASSES 2
#if 1
	for (int y = 0; y < output.getHeight(); ++y) {
		//fprintf(stderr, "\r%d%%", (100 * y + output.getHeight() / 2) / output.getHeight());
		for (int x = 0; x < output.getWidth(); ++x) {
			fprintf(stderr, "\rgenerating %.2f%%", 100.0 * (y * output.getWidth() + x) / (output.getHeight() * output.getWidth()));
			double best_score = INFINITY;
			int best_color_ix[4] = { 0, 1, 2, bgcolorix };
			Block& input_block = image->blockAt(x, y);
			Block& output_block = output.blockAt(x, y);

			output_block.setWidth(input_block.getWidth());

			input_block.setPaletteSize(num_colors_per_block + num_global_colors);
			output_block.setPaletteSize(num_colors_per_block + num_global_colors);

			// XXX in multicolor mode, bit pattern 0b00 is used for the background color
			input_block.setPalette(3, bgcolor);
			output_block.setPalette(3, bgcolor);

			for (int color0ix = 0; color0ix < color0max; ++color0ix) {
				if (exclusions[color0ix]) {
					continue;
				}
				input_block.setPalette(0, palette->colors[color0ix]);
				output_block.setPalette(0, palette->colors[color0ix]);

				for (int color1ix = color0ix; color1ix < color1max; ++color1ix) {
					if (exclusions[color1ix]) {
						continue;
					}
					input_block.setPalette(1, palette->colors[color1ix]);
					output_block.setPalette(1, palette->colors[color1ix]);

					int color2ix = color1ix;
					if (gfx_mode == HIRES) {
						color2ix = num_palette_colors - 1;
					}

					for (; color2ix < num_palette_colors; ++color2ix) {
						if (gfx_mode == MULTICOLOR && exclusions[color2ix]) {
							continue;
						}
						input_block.setPalette(2, palette->colors[color2ix]);
						output_block.setPalette(2, palette->colors[color2ix]);

						generate(&input_block, &output_block);
						//refine_genetically(&input_block, &output_block, BLOCK_PASSES);

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

			for (int c = 0; c < num_colors_per_block + num_global_colors; ++c) {
				input_block.setPalette(c, palette->colors[best_color_ix[c]]);
				output_block.setPalette(c, palette->colors[best_color_ix[c]]);
			}
			generate(&input_block, &output_block);
		}
	}
#else

	// test each color for its goodness
	// (experimental)
	// (not very good yet, but may be good with tweaking)
	{
		for (int y = 0; y < image->getHeight(); ++y) {
			for (int x = 0; x < image->getWidth(); ++x) {
				Block& input_block = image->blockAt(x, y);
				Block& output_block = output.blockAt(x, y);

				output_block.setWidth(block_width);

				input_block.setPaletteSize(num_colors_per_block + num_global_colors);
				output_block.setPaletteSize(num_colors_per_block + num_global_colors);

				// XXX in multicolor mode, bit pattern 0b00 is used for the background color
				//input_block.setPalette(3, bgcolor);
				//output_block.setPalette(3, bgcolor);

				int best_color_ix[4] = { 0, 0, 0, bgcolorix };
				double goodnesses[NUM_COLORS] = {};
				for (int py = 0; py < input_block.getHeight(); ++py) {
					for (int px = 0; px < input_block.getWidth(); ++px) {
						const auto& color = input_block.pixelAt(px, py);
						// mix 2 colors in different amounts
						for (int ix0 = 0; ix0 < NUM_COLORS; ++ix0) {
							if (exclusions[ix0]) {
								continue;
							}
							const auto& testColor0 = palette->colors[ix0];
							for (int ix1 = 0; ix1 < NUM_COLORS; ++ix1) {
								if (ix0 == ix1 || exclusions[ix1]) {
									continue;
								}
								const auto& testColor1 = palette->colors[ix1];
								for (int mix = 0; mix < 9; ++mix) {
									double t = mix / 9.;
									Color testColor = (testColor0 * t) + (testColor1 * (1 - t));
									double goodness = calc_color_goodness(color, testColor);
									goodnesses[ix0] += pow(      t, 4.0) * goodness;
									goodnesses[ix1] += pow((1 - t), 4.0) * goodness;
								}
							}
						}
					}
				}
				if (true) {
					fprintf(stderr, "block (%2d, %2d) color goodnesses:", x, y);
					for (int ix = 0; ix < NUM_COLORS; ++ix) {
						fprintf(stderr, " %6.0f", goodnesses[ix]);
					}
					fprintf(stderr, "  best:");
				}
				for (int i = 0; i < num_colors_per_block; ++i) {
					int best_color = 0;
					for (int ix = 1; ix < NUM_COLORS; ++ix) {
						if (goodnesses[ix] > goodnesses[best_color]) {
							best_color = ix;
						}
					}
					best_color_ix[i] = best_color;
					goodnesses[best_color] = -INFINITY; // don't pick this as the next best color
					fprintf(stderr, " %2d", best_color);
				}
				fprintf(stderr, "\n");
				for (int c = 0; c < num_colors_per_block + num_global_colors; ++c) {
					input_block.setPalette(c, palette->colors[best_color_ix[c]]);
					output_block.setPalette(c, palette->colors[best_color_ix[c]]);
				}

				generate(&input_block, &output_block);
				//refine_genetically(&input_block, &output_block, BLOCK_PASSES);
			}
		}

	}

#endif

#if 1
	// refine
			//fprintf(stderr, "\rrefining   %.2f%%", 100.0 * (y * 8 + x) / (8 * 8));
	refine_genetically(image, &output);
#endif

	fprintf(stderr, "\r%.2f%%", 100.0);
	fprintf(stderr, "\n");

	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);

	save_image(stdout, &output);
	//fprintf(stderr, "%s (%d)\n", __FILE__, __LINE__);
#endif

	return 0;
}

