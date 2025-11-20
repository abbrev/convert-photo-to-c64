#ifndef COLOR_H
#define COLOR_H

struct Color {
	double r,g,b;

	bool operator ==(const Color &other) const {
		return r == other.r &&
		       g == other.g &&
		       b == other.b;
	}

	bool operator !=(const Color &other) const { return !(*this == other); }
};

#endif
