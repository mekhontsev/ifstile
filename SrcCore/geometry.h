// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include "ims_random.h"
#include "ims_num_traits.h"
#include "ball3d.h"
#include "dyn_mat_vec.h"

template<typename Real>
struct Geometry
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	using Mat3 = Eigen::Matrix<Real, 3, 3>;

	static void set_rotate_ex(Mat3& dst, const Vec3& q, Real ang, Real k1, Real k2)
	{
		dst.setIdentity();

		Real sn, cs;
		sn = k2*sin(ang);
		cs = k2*cos(ang);

		Vec3 p=q;
		p.normalize();

		Vec3 a1, b1, c1, a2, b2, c2;

		a1 = b1 = c1 = p;
		a1 *= p(0); b1 *= p(1); c1 *= p(2);
		
		a2 = Vec3(0, p(2), -p(1))*sn;
		b2 = Vec3(-p(2), 0, p(0))*sn;
		c2 = Vec3(p(1), -p(0), 0)*sn;

		auto a = dst.row(0);
		auto b = dst.row(1);
		auto c = dst.row(2);

		a = a1*k1;
		b = b1*k1;
		c = c1*k1;
		
		a1(0) -= 1;
		b1(1) -= 1;
		c1(2) -= 1;

		a1 *= cs;
		b1 *= cs;
		c1 *= cs;

		a -= a1;
		b -= b1;
		c -= c1;
		
		a -= a2;
		b -= b2;
		c -= c2;
	};
};

template<typename Real>
struct ray
{
	DynVec<Real> p;//location
	DynVec<Real> d;//direction
};


//n1*x1+n2*x2+....+d = 0
template<typename Real>
struct hyperplane
{
	DynVec<Real> n;//unit normal
	Real d;
};


template<typename Real>
struct light_params 
{
	struct lsource 
	{
		using Vec3 = Eigen::Matrix<Real, 3, 1>;

		bool is_direct;
		Real brightness;
		Vec3 p;//location or direction
	};

	Real m_ambient;	//ambient brightness
	Real m_camera;	//camera brightness

	std::vector<lsource> m_lsource;

	bool empty() const
	{
		return m_ambient == 0 && m_camera == 0 && m_lsource.empty();
	};

	void clear() 
	{
		m_ambient = 0;
		m_camera = 0;
		m_lsource.clear();
	};
};


//camera position in 3D
template<typename Real>
struct camera 
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;
	using Mat3 = Eigen::Matrix<Real, 3, 3>;
	
	Vec3 m_loc;
	Vec3 m_ref;
	Vec3 m_ver;

	//vertical viewing angle in degrees
	Real m_fov=30;



	//the tangent of the angle between dir and the vector from pos to the center of the screen
	//if not in stereo mode, then 0
	Real m_stereo = 0;

	//screen orientation matrix, transform from world to screen
	//m.a = horizontal screen direction
	//m.b = vertical screen direction
	//m.c = screen normal
	Mat3 m;

	void clear2() { m_fov = 0; };
	bool empty2() const { return m_fov <= 0; };


	bool init()
	{
#if 0
		{
			Mat3 dst;
			Vec3 q = Vec3(1, 0, 1);
			Geometry<Real>::set_rotate_ex(dst, q, 1, 0.5, 0.5);
		}
#endif
		
		
		if (m_fov <= 0)return false;

		if (m_ver.norm() < ims_num_traits<Real>::almost_zero())return false;

		Vec3 c = m_ref - m_loc;
		c.normalize();

		Vec3 b = m_ver - c*(c.dot(m_ver));
		b.normalize();

		Vec3 a = -b.cross(c);

		m.row(0) = a;
		m.row(1) = b;
		m.row(2) = c;

		return true;
	};

	Vec3 adjusted_ver() const 
	{
		Vec3 c = m_ref - m_loc;
		c.normalize();

		Vec3 b = m_ver - c * (c.dot(m_ver));
		b.normalize();
		return b;
	}

	void wheel_dist(Real delta)
	{
		Real q = std::pow(2, delta / 10);
		auto rdir = m_loc - m_ref;
		rdir *= q;
		m_loc = rdir + m_ref;
	}

	void translate(Real dx, Real dy)
	{
		auto lc = m_loc - m_ref;
		auto rdir = lc.cross(m_ver);
		Real step = lc.norm();

		rdir.normalize();
		auto vdir = m_ver;
		vdir.normalize();

		dx *= step;
		dy *= step;

		vdir *= dy;
		rdir *= dx;

		m_ref += vdir;
		m_ref += rdir;

	}

	void roll(Real /*ang*/) {

	}

	void rotate(Real dx, Real dy, bool change_target)
	{
		Vec3 lc = m_loc - m_ref;
		Vec3 rdir = lc.cross(m_ver);
		
		Mat3 mq;
		
		let p2 = boost::math::constants::pi<Real>() * 2;

		//rotation around the vertical axis
		Geometry<Real>::set_rotate_ex(mq, m_ver, dy * p2, 1, 1);
		if (change_target) {
			m_ref = mq * (m_ref - m_loc) + m_loc;
		} else {
			m_loc = mq * (m_loc - m_ref) + m_ref;
		}

		//rotation relative to the right direction
		Geometry<Real>::set_rotate_ex(mq, rdir, dx * p2, 1, 1);
		if (change_target) {
			m_ref = mq * (m_ref - m_loc) + m_loc;
		} else {
			m_loc = mq * (m_loc - m_ref) + m_ref;
		}
	
		m_ver = mq*m_ver;
		m_ver = adjusted_ver();
	
	}

	//set randomly relative to the ball
	void randomize(const ball3d<Real>& b)
	{
		auto& rng = ims_random::get().rng;
		std::uniform_real_distribution<Real> distr(-1, 1);

		Eigen::Vector3d cdir;
		cdir << distr(rng), distr(rng), distr(rng);
		cdir.normalize();
		cdir *= b.r*sqrt(2);

		//don't touch FOV
		m_ref << b.c(0), b.c(1), b.c(2);
		m_loc = m_ref + cdir;

		m_ver << distr(rng), distr(rng), distr(rng);
		m_ver.normalize();

		init();
	}

};


struct scr_point
{
	size_t x, y;
};


//drawing area parameters
struct screen_area
{
	size_t m_res[2];			//screen resolution

	
	
	size_t m_num = 0;				//number of thumbnails

	

	size_t get_nx() const { return m_res[0] / m_tx; };
	size_t get_ny() const { return m_res[1] / m_ty; };
	size_t get_num() const { return m_num; };


	bool empty() const { return m_res[0] == 0 || m_res[1] == 0; };
	
	//get the coordinates of the thumbnail position by index
	void get_xy(size_t i, size_t& ix, size_t& iy) const
	{
		let nx = get_nx();

		ix = i % nx;
		iy = i / nx;
	};

	//get the thumbnail number based on the screen point coordinates
	size_t get_idx(size_t px, size_t py) const
	{
		let nx = get_nx();
		let ny = get_ny();

		if (nx==0)return 0;

		let ix = px / m_tx;
		let iy = py / m_ty;

		return (ny-1-iy)*nx + ix;
	};

	void set_res(size_t w, size_t h)
	{
		assert(w > 0 && h > 0);
		m_res[0] = w;
		m_res[1] = h;
	};

	size_t calc_thumb(size_t& hx, size_t& hy,
		size_t max_num_sets, size_t max_thmb) const
	{
		let w = m_res[0];
		let h = m_res[1];

		if (max_num_sets <= 1) {
			hx = w;
			hy = h;
			return 1;
		}

		size_t nx = 1, ny = 1;

		for (;;) {

			let num = nx * ny;

			let x = w / nx;
			let y = h / ny;

			bool ready = false;

			if (max_num_sets <= num) {
				ready = true;
			} else {
				if (w / (nx + 1) > h / (ny + 1)) {
					++nx;
				} else {
					++ny;
				}
				if (std::max(ny, nx) > max_thmb) {
					ready = true;
				}
			}

			if (ready) {
				hx = x;
				hy = y;
				return num;
			}
		}
	
	};

	//tries to make thumbnails as square as possible
	void initX(size_t max_num_sets, size_t max_thmb)
	{
		m_num = calc_thumb(m_tx, m_ty, max_num_sets, max_thmb);
	}

	size_t get_tx() const { return m_tx; };
	size_t get_ty() const { return m_ty; };
private:
	size_t m_tx = 0, m_ty = 0;	//size of one thumbnail
};


template<typename Real>
struct cam_proj
{
	using Vec3 = Eigen::Matrix<Real, 3, 1>;

	//the tangent of half the vertical viewing angle
	//takes values from zero to infinity
	//for example, 1 means a viewing angle of [-45;45] degrees
	Real m_tf2_ver;

	//Projection parameters
	//x,y - coordinates of the lower-left corner of the image (in the camera coordinate system)
	//z - distance from the observer to the screen plane
	//so that the pixels have a size of 1
	Vec3 m_prj;

	//projection dimensions in pixels
	size_t m_prj_pix[2];


	void init(const camera<Real>& cam, const size_t w, const size_t h)
	{
		m_tf2_ver = tan(cam.m_fov*(boost::math::constants::pi<Real>() / 360));
		
		m_prj_pix[0] = w;
		m_prj_pix[1] = h;

		let& m= cam.m;

		m_prj = m.row(2) + m.row(0)*cam.m_stereo;

		//multiply by the distance to the screen plane
		m_prj *= Real(.5)*h / m_tf2_ver;

		//translate into the camera coordinate system
		m_prj = m*m_prj;

		Real d = m_prj.norm();
		m_prj[0] = m_prj[0] * d / m_prj[2] - Real(w / 2);
		m_prj[1] = m_prj[1] * d / m_prj[2] - Real(h / 2);
		m_prj[2] = d;
	};

	//project a point from the world coordinate system to the screen coordinate system
	//output: dst - coordinates of the point in the screen coordinate system (centered at the observer)
	//dst.x,dst.y - projection of the point onto the screen
	//dst.z - square of the depth
	//returns false if the point is NOT IN FRONT of the observer (in which case x,y are undefined)
	bool get_proj(const camera<Real>& cam, Vec3& dst, const Vec3& src) const
	{
		let& m = cam.m;

		Vec3 p = m*(src - cam.m_loc);

		if (p[2] <= 0)return false;

		Real q = m_prj[2] / p[2];
		Real nr2 = p.squaredNorm();

		dst = Vec3(p[0] * q - m_prj[0], p[1] * q - m_prj[1], nr2);

		return true;
	}

	//returns the vector from the observer to the pixel (p.x,p.y)
	//the output vector is not normalized!
	Vec3 back_proj_dir(const camera<Real>& cam, size_t ix, size_t iy)  const
	{
		Vec3 p = m_prj;
		p(0) += Real(ix);
		p(1) += Real(iy);
		return cam.m.transpose()*p;
	};

};


//information about the subspace in which we are constructing the image
template<typename Real>
struct subspace_info
{
	//the basis of the subspace in which we are constructing (2 or 3 columns)
	//the first index is dim_set, the second index is 2 or 3
	DynMat<Real> basis_user;
	//the origin of the subspace
	DynVec<Real> origin;

	//orthonormal basis
	DynMat<Real> basis;

	//section dimension (2 or 3)
	size_t get_section_dim() const { return (size_t)basis_user.cols(); };

	//dimensionality of space or entire set
	size_t get_dim_space() const { return (size_t)basis_user.rows(); };

	//modified Gram-Schmidt process
	void init_si() 
	{
		//TODO: check
		//basis = basis_user.householderQr().householderQ();
		
		basis = basis_user;

		for (size_t i = 0; i < (size_t)basis.cols(); ++i) {
			auto a = basis.col(i);
			for (size_t j = 0; j < i; ++j) {
				let b= basis.col(j);//normalized on previous iterations
				a -= a.dot(b)*b;
			}
			a.normalize();
		}
	};



	void reset() 
	{
		origin.setZero();

		basis_user.setZero();

		let n = (size_t)std::min(basis_user.rows(), basis_user.cols());
		for (size_t i = 0; i < n; ++i) {
			basis_user(i, i) = 1;
		}
	};


	void resize2(size_t dim_set, size_t dim_rend)
	{
		dim_rend = ims_clamp(dim_rend, 2, 3);
		dim_rend = std::min(dim_rend, dim_set);

		origin.resize(dim_set);
		basis_user.resize(dim_set, dim_rend);
	};
	

	//o'=m(o-c)+c

	void rotate_base(Real c, Real s, Real& cx, Real& cy) 
	{
		let n = sqrt(c * c + s * s);
		if (n == 0)return;
		c /= n;
		s /= n;

		Eigen::Matrix<Real, 2, 2> m;
		m << c, -s, s, c;
		basis_user = basis*m.transpose();

		Eigen::Matrix<Real, 2, 1> center;
		center << cx, cy;


		center = m * center;
		cx = center(0);
		cy = center(1);

		init_si();
	}

};




template<typename Real>
struct screen_params
{
	Real x, y;	//center
	Real ps;	//pixel size
	Real a;		//rotation angle in degrees
};

template<typename Real>
struct screen_disk
{
	Real c[2];	//center of the screen
	Real r=-1;	//radius of the inscribed circle
	Real a = 0;	//rotation angle in degrees

	void clear2(){r = -1;};
	bool empty2() const  {return r <= 0;}

	//get pixel size
	Real get_ps(size_t w, size_t h) const
	{
		return r * 2 / std::min(w, h);
	}

	//set pixel size
	void set_ps(size_t w, size_t h, Real ps)
	{
		r=ps* std::min(w, h) /2;
	}


	void to_params(screen_params<Real>& sp, size_t w, size_t h) const
	{
		sp.ps = get_ps(w,h);
		sp.x = c[0];
		sp.y = c[1];
		sp.a = a;
	}
};


struct camera_ex
{
	using Real = double;

	screen_disk<double> m_sd;//2d
	camera<double> m_camera;//3d

	bool m_3d_empty = true;
	bool m_2d_empty = true;

	bool empty(size_t dim = 0) const
	{
		if (dim == 2)return m_2d_empty;
		if (dim == 3)return m_3d_empty;
		return m_2d_empty && m_3d_empty;
	}

	void clear(size_t dim = 0)
	{
		if (dim == 2)m_2d_empty = true;
		else if (dim == 3)m_3d_empty = true;
		else {
			m_2d_empty = true;
			m_3d_empty = true;
		};
	}
};


struct screen_rect
{
	using ut = int;
	ut c[2];//upper left corner
	ut s[2];//size

	ut width() const { return s[0]; };
	ut height() const { return s[1]; };

	void clear()
	{
		c[0] = c[1] = s[0] = s[1] = 0;
	};
	bool empty() const { return s[0] == 0 || s[1] == 0; };

	bool contain2(ut x, ut y) const
	{
		return 	
			x >= c[0] && x < c[0] + s[0] &&
			y >= c[1] && y < c[1] + s[1];
	}

	void to_image2(ut& sx, ut& sy) const
	{
		sy = c[1] + s[1] - 1 - sy;//flip vertical
		sx -= c[0];
	}
};


////////////////////////////////////////////////////////////////////////////////

template<typename Real>
void mul_colors(
	std::array<Real, 5>& dc, 
	const std::array<Real, 5>& lc, 
	const std::array<Real, 5>& rc)
{
	if (lc[3] == 0) {
		dc = rc;
		return;
	}
	if (rc[3] == 0) {
		dc = lc;
		return;
	}
	let sa = lc[3];
	let da = rc[3];
	let sd = sa + da;
	dc[3] = sd / 2.0f;
	let m = 1.0f / sd;
	for (size_t i = 0; i < 3; ++i) {
		dc[i] = m * (lc[i] * sa + rc[i] * da);
	}
	dc[4] = lc[4];//take from the left operand
};




template<typename Real>
struct style
{
	//palette indices - appended during composition
	boost::container::small_vector<Real, 2> pal_idx;
};




