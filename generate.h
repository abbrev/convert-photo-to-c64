#ifndef GENERATE_H
#define GENERATE_H

#include <stdbool.h>

#include "image.h"
#include "block.h"

void generate(const Block *src, const Color colors[2], Block *gen);
void refine_genetically(const Block *src, const Color colors[2], Block *gen, int passes);

void refine_genetically(const Image *src, const Color colors[200][200][2], Image *gen);

#endif
