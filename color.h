#ifndef COLOR_H
#define COLOR_H

struct Color {
	double r,g,b;
	int ix;

	Color() : r{}, g{}, b{}, ix{} {}

	Color(double r, double g, double b, int ix) : r{r}, g{g}, b{b}, ix{ix} {}

	Color(const Color &other) {
		r = other.r; g = other.g; b = other.b; ix = other.ix;
	}

	bool operator ==(const Color &other) const {
		return r == other.r &&
		       g == other.g &&
		       b == other.b;
	}

	bool operator !=(const Color &other) const { return !(*this == other); }

	Color operator -(const Color &other) const { return (Color){ r - other.r, g - other.g, b - other.b, 0 }; }
	Color operator +(const Color &other) const { return (Color){ r + other.r, g + other.g, b + other.b, 0 }; }
	Color operator /(double d) const { return { r / d, g / d, b / d, 0 }; }
	Color operator *(double d) const { return { r * d, g * d, b * d, 0 }; }

	Color& operator +=(const Color &other) { *this = *this + other; return *this; }
	Color& operator /=(double d) { *this = *this / d; return *this; }

	Color& operator =(const Color &other) { r = other.r; g = other.g; b = other.b; ix = other.ix; return *this; }
};

#endif
