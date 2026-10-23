#ifndef COLOR_H
#define COLOR_H

struct Color {
	Color() :
		r_{},
		g_{},
		b_{},
		ix_{}
	{
		//rgb_to_yuv();
	}

	Color(double r, double g, double b, int ix) :
		r_{r},
		g_{g},
		b_{b},
		ix_{ix}
	{
		//rgb_to_yuv();
	}

	Color(const Color &other) :
		r_{other.r_},
		g_{other.g_},
		b_{other.b_},
		ix_{other.ix_},
		yuv{other.yuv},
		oklab{other.oklab},
		ydbdr{other.ydbdr}
	{}

	bool operator ==(const Color &other) const {
		return r_ == other.r_ &&
		       g_ == other.g_ &&
		       b_ == other.b_;
	}

	bool operator !=(const Color &other) const { return !(*this == other); }

	Color operator -(const Color &other) const { return Color(r_ - other.r_, g_ - other.g_, b_ - other.b_, 0); }
	Color operator +(const Color &other) const { return Color(r_ + other.r_, g_ + other.g_, b_ + other.b_, 0); }
	Color operator /(double d) const { return Color(r_ / d, g_ / d, b_ / d, 0); }
	Color operator *(double d) const { return Color(r_ * d, g_ * d, b_ * d, 0); }

	Color& operator +=(const Color &other) { *this = *this + other; return *this; }
	Color& operator /=(double d) { *this = *this / d; return *this; }

	Color& operator =(const Color &other) {
		r_ = other.r_;
		g_ = other.g_;
		b_ = other.b_;
		ix_ = other.ix_;
		yuv = other.yuv;
		oklab = other.oklab;
		ydbdr = other.ydbdr;
		return *this;
	}

	bool operator<(const Color &other) const { return ydbdr.y < other.ydbdr.y; }

	double r()  const { return r_;  }
	double g()  const { return g_;  }
	double b()  const { return b_;  }

	int ix() const { return ix_; }

	double yuv_y()  const { if (!yuv.initialized) { rgb_to_yuv(); } return yuv.y;  }
	double yuv_u()  const { if (!yuv.initialized) { rgb_to_yuv(); } return yuv.u;  }
	double yuv_v()  const { if (!yuv.initialized) { rgb_to_yuv(); } return yuv.v;  }

	double oklab_l()  const { if (!oklab.initialized) { rgb_to_oklab(); } return oklab.l;  }
	double oklab_a()  const { if (!oklab.initialized) { rgb_to_oklab(); } return oklab.a;  }
	double oklab_b()  const { if (!oklab.initialized) { rgb_to_oklab(); } return oklab.b;  }

	double ycbcr_y()  const { if (!ycbcr.initialized) { rgb_to_ycbcr(); } return ycbcr.y;  }
	double ycbcr_cb() const { if (!ycbcr.initialized) { rgb_to_ycbcr(); } return ycbcr.cb;  }
	double ycbcr_cr() const { if (!ycbcr.initialized) { rgb_to_ycbcr(); } return ycbcr.cr;  }

	double ydbdr_y()  const { if (!ydbdr.initialized) { rgb_to_ydbdr(); } return ydbdr.y;  }
	double ydbdr_db() const { if (!ydbdr.initialized) { rgb_to_ydbdr(); } return ydbdr.db;  }
	double ydbdr_dr() const { if (!ydbdr.initialized) { rgb_to_ydbdr(); } return ydbdr.dr;  }

private:
	void rgb_to_oklab() const {
		double l = +0.4122214708*r_ +0.5363325363*g_ +0.0514459929*b_;
		double m = +0.2119034982*r_ +0.6806995451*g_ +0.1073969566*b_;
		double s = +0.0883024619*r_ +0.2817188376*g_ +0.6299787005*b_;

		double lp = cbrt(l);
		double mp = cbrt(m);
		double sp = cbrt(s);

		oklab.l = +0.2104542553*lp +0.7936177850*mp -0.0040720468*sp;
		oklab.a = +1.9779984951*lp -2.4285922050*mp +0.4505937099*sp;
		oklab.b = +0.0259040371*lp +0.7827717662*mp -0.8086757660*sp;
		oklab.initialized = true;
	}

	void rgb_to_ycbcr() const {
		static const double gamma = 1/2.2;
		double r = pow(r_, gamma);
		double g = pow(g_, gamma);
		double b = pow(b_, gamma);
		ycbcr.y  = +0.2126*r +0.7152*g +0.0722*b;
		ycbcr.cb = -0.1146*r -0.3854*g +0.5000*b;
		ycbcr.cr = +0.5000*r -0.4542*g -0.0458*b;
		ycbcr.initialized = true;
	}

	void rgb_to_yuv() const {
		// Y'UV
		static const double gamma = 1/2.2;
		double r = pow(r_, gamma);
		double g = pow(g_, gamma);
		double b = pow(b_, gamma);
		yuv.y = +0.29900*r +0.58700*g +0.11400*b;
		yuv.u = -0.14713*r -0.28886*g +0.43600*b;
		yuv.v = +0.61500*r -0.51499*g -0.10001*b;
		yuv.initialized = true;
	}

	void rgb_to_ydbdr() const {
		// YDbDr
		// XXX YDbDr is linear, but I'm testing various gammas/inverse
		// gammas. The intention here is to give more weight to darker
		// colors.
		//
		// optimum value seems to be somewhere around 1.4 or 1.6; y
		// weight can be decreased a bit too.
		static const double gamma = 1/1.4;

		double r = r_; //pow(r_, gamma);
		double g = g_; //pow(g_, gamma);
		double b = b_; //pow(b_, gamma);

		ydbdr.y  = +0.299*r +0.587*g +0.114*b;
		ydbdr.db = -0.450*r -0.883*g +1.333*b;
		ydbdr.dr = -1.333*r +1.116*g +0.217*b;

		ydbdr.initialized = true;
	}

	double r_,g_,b_;
	int ix_;

	mutable struct {
		double y,u,v;
		bool initialized = false;
	} yuv;
	mutable struct {
		double l,a,b;
		bool initialized = false;
	} oklab;
	mutable struct {
		double y, cb, cr;
		bool initialized = false;
	} ycbcr;
	mutable struct {
		double y, db, dr;
		bool initialized = false;
	} ydbdr;
};

#endif
