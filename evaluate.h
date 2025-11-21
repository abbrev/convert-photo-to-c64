#ifndef EVALUATE_H
#define EVALUATE_H

#include "image.h"

double evaluate_color(const Color& src, const Color& gen);
double evaluate_single_pixel(const Block *src, const Block *gen, int x, int y);
double evaluate_pixel(const Block *src, const Block *gen, int x, int y);
double evaluate_pixel(const Image *src, const Image *gen, int x, int y);
double evaluate(const Block *src, const Block *gen);
double evaluate(const Image *src, const Image *gen);

void set_dither_weight(double w);

#endif
