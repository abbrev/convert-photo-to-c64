#ifndef COLOR_H
#define COLOR_H

struct Color {
	Color() :
		r_{},
		g_{},
		b_{},
		ix_{}
	{
		rgb_to_yuv();
	}

	Color(double r, double g, double b, int ix) :
		r_{r},
		g_{g},
		b_{b},
		ix_{ix}
	{
		rgb_to_yuv();
	}

	Color(const Color &other) :
		r_{other.r_},
		g_{other.g_},
		b_{other.b_},
		ix_{other.ix_},
		yuv{other.yuv},
		ipt{other.ipt},
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
		ipt = other.ipt;
		oklab = other.oklab;
		ydbdr = other.ydbdr;
		return *this;
	}

	bool operator<(const Color &other) const { return ydbdr.y < other.ydbdr.y; }

	double r()  const { return r_;  }
	double g()  const { return g_;  }
	double b()  const { return b_;  }

	int ix() const { return ix_; }

	double yuv_y()  const { return yuv.y;  }
	double yuv_u()  const { return yuv.u;  }
	double yuv_v()  const { return yuv.v;  }

	double ipt_i()  const { return ipt.i;  }
	double ipt_p()  const { return ipt.p;  }
	double ipt_t()  const { return ipt.t;  }

	double oklab_l()  const { return oklab.l;  }
	double oklab_a()  const { return oklab.a;  }
	double oklab_b()  const { return oklab.b;  }

	double ydbdr_y()  const { return ydbdr.y;  }
	double ydbdr_db() const { return ydbdr.db;  }
	double ydbdr_dr() const { return ydbdr.dr;  }

private:
	void rgb_to_yuv() {
		// YUV
		if (false) {
			// based on Y'UV and YDbDr
			yuv.y = + 0.29900*r_ + 0.58700*g_ + 0.11400*b_;
			yuv.u = - 0.14713*r_ - 0.28886*g_ + 0.43600*b_;
			yuv.v = + 0.61500*r_ - 0.51499*g_ - 0.10001*b_;
		}

		if (false) {
			double x = +0.49000*r_ +0.31000*g_ +0.20000*b_;
			double y = +0.17697*r_ +0.81240*g_ +0.01063*b_;
			double z = +0.00000*r_ +0.01000*g_ +0.99000*b_;

			// IPT
			if (false) {
				double l = +0.4002*x +0.7075*y -0.0807*z;
				double m = -0.2280*x +1.1500*y +0.0612*z;
				double s = +0.0000*x +0.0000*y +0.9184*z;

				double lp = l >= 0 ? pow(l, 0.43) : -pow(-l, 0.43);
				double mp = m >= 0 ? pow(m, 0.43) : -pow(-m, 0.43);
				double sp = s >= 0 ? pow(s, 0.43) : -pow(-s, 0.43);

				ipt.i = +0.4000*lp +0.4000*mp +0.2000*sp;
				ipt.p = +4.4550*lp -4.8510*mp +0.3960*sp;
				ipt.t = +0.8056*lp +0.3572*mp -1.1628*sp;
			}

			// OKLab
			{
				double l = +0.4122214708*r_ +0.5363325363*g_ +0.0514459929*b_;
				double m = +0.2119034982*r_ +0.6806995451*g_ +0.1073969566*b_;
				double s = +0.0883024619*r_ +0.2817188376*g_ +0.6299787005*b_;

				double lp = cbrt(l);
				double mp = cbrt(m);
				double sp = cbrt(s);

				oklab = {
					+0.2104542553*lp + 0.7936177850*mp - 0.0040720468*sp,
					+1.9779984951*lp - 2.4285922050*mp + 0.4505937099*sp,
					+0.0259040371*lp + 0.7827717662*mp - 0.8086757660*sp,
				};
			}
		}

		{
			// YDbDr
			ydbdr.y  = +0.299*r_ +0.587*g_ +0.114*b_;
			ydbdr.db = -0.450*r_ -0.883*g_ +1.333*b_;
			ydbdr.dr = -1.333*r_ +1.116*g_ +0.217*b_;
		}
	}

	double r_,g_,b_;
	int ix_;

	struct {
		double y,u,v;
	} yuv;
	struct {
		double i,p,t;
	} ipt;
	struct {
		double l,a,b;
	} oklab;
	struct {
		double y, db, dr;
	} ydbdr;
};

#endif
