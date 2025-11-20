#ifndef COLOR_H
#define COLOR_H

struct Color {
	uint8_t r,g,b;

	bool operator ==(const Color &other) const {
		return r == other.r &&
		       g == other.g &&
		       b == other.b;
	}
};

#endif
