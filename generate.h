#ifndef GENERATE_H
#define GENERATE_H

#include <stdbool.h>

#include "block.h"

void generate(const Block *src, const Color colors[2], Block *gen, bool finalpass);

#endif
