#ifndef COLOR_H
#define COLOR_H

struct Color {
private:
	double r_,g_,b_;
	int ix_;

	double y_,u_,v_;

public:

	Color() : r_{}, g_{}, b_{}, ix_{}, y_{}, u_{}, v_{} {}

	Color(double r, double g, double b, int ix) : r_{r}, g_{g}, b_{b}, ix_{ix} {
		rgb_to_yuv();
	}

	Color(const Color &other) : r_{other.r_}, g_{other.g_}, b_{other.b_}, ix_{other.ix_}, y_{other.y_}, u_{other.u_}, v_{other.v_} {}

	bool operator ==(const Color &other) const {
		return r_ == other.r_ &&
		       g_ == other.g_ &&
		       b_ == other.b_;
	}

	bool operator !=(const Color &other) const { return !(*this == other); }

	Color operator -(const Color &other) const { return { r_ - other.r_, g_ - other.g_, b_ - other.b_, 0 }; }
	Color operator +(const Color &other) const { return { r_ + other.r_, g_ + other.g_, b_ + other.b_, 0 }; }
	Color operator /(double d) const { return { r_ / d, g_ / d, b_ / d, 0 }; }
	Color operator *(double d) const { return { r_ * d, g_ * d, b_ * d, 0 }; }

	Color& operator +=(const Color &other) { *this = *this + other; return *this; }
	Color& operator /=(double d) { *this = *this / d; return *this; }

	Color& operator =(const Color &other) { r_ = other.r_; g_ = other.g_; b_ = other.b_; ix_ = other.ix_; y_ = other.y_; u_ = other.u_; v_ = other.v_; return *this; }

	bool operator<(const Color &other) const { return y_ < other.y_; }

	double r()  const { return r_;  }
	double g()  const { return g_;  }
	double b()  const { return b_;  }
	double ix() const { return ix_; }
	double y()  const { return y_;  }
	double u()  const { return u_;  }
	double v()  const { return v_;  }

private:
	void rgb_to_yuv() {
		double r = r_; //pow(this->r_, 1/2.2);
		double g = g_; //pow(this->g_, 1/2.2);
		double b = b_; //pow(this->b_, 1/2.2);

		y_ = + 0.29900 * r + 0.58700 * g + 0.11400 * b;
		u_ = - 0.14713 * r - 0.28886 * g + 0.43600 * b;
		v_ = + 0.61500 * r - 0.51499 * g - 0.10001 * b;
	}
};

#endif
