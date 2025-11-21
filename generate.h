#ifndef GENERATE_H
#define GENERATE_H

#include <stdbool.h>

#include "image.h"
#include "block.h"

void generate(const Block *src, Block *gen);
void refine_genetically(const Block *src, Block *gen, int passes, bool swap = true);

void refine_genetically(const Image *src, Image *gen);

#endif
