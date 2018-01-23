#ifndef EVALUATE_H
#define EVALUATE_H

#include "block.h"

long evaluate_color(const Color *src, const Color *gen);
long evaluate_single_pixel(const Block *src, const Block *gen, int x, int y);
long evaluate_pixel(const Block *src, const Block *gen, int x, int y);
long evaluate(const Block *src, const Block *gen);

#endif
