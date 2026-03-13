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

#include "pch.h"
#include "ims_to_string_ex.h"
#include "flame_calc.h"
#include "palette.h"
#include "version.h"
#include "ims_random.h"
#include "ims_num_traits.h"
#include "edge_map.h"
#include "geometry.h"
#include "ims_graph_base.h"
#include "ims_val.h"

#define nlc "\n"

using Real = double;

static void random_rotate3d(DynMat<Real>& rm)
{
	DynMat<Real> rot(3, 3);
	auto& rng = ims_random::getR().rng;
	std::uniform_real_distribution<Real>
		distr(0, 2 * boost::math::constants::pi<Real>());

	Real a, c, s;
	//////////////////////
	a = distr(rng); c = cos(a); s = sin(a);
	rot <<
		c, -s, 0,
		s, c, 0,
		0, 0, 1;
	rm *= rot;

	//////////////////////
	a = distr(rng); c = cos(a); s = sin(a);
	rot <<
		1, 0, 0,
		0, c, -s,
		0, s, c;
	rm *= rot;

	//////////////////////
	a = distr(rng); c = cos(a); s = sin(a);
	rot <<
		c, 0, -s,
		0, 1, 0,
		s, 0, c;
	rm *= rot;
};


void ims_to_flame(
	std::ostream& str,
	size_t dim,
	std::string_view name,
	std::span<const flame_edge> fedegs,
	const size_t ver,
	std::span<const edge_map> ri,
	const screen_disk<Real>& sd,
	const palette& pal,
	const std::array<float, 4>& background
)
{
	ims_precision prec(str);
	prec.template max<Real>();

	using flame_map = std::array<Real, 6>;
	std::vector<flame_map> maps;
	
	if (dim == 3) {
		maps.resize(ri.size() * 3);

		let eps = ims_num_traits<Real>::almost_zero();
		for (size_t tr = 0; tr < 3; ++tr) {
			DynMat<Real> qm(3, 3), rm(3, 3);

			qm.setIdentity();
			rm.setIdentity();
			if (tr > 0) {
				random_rotate3d(qm);
				rm = qm.inverse();
			}
			DynMat<Real> A;
			DynVec<Real> bt;

			size_t idx = 0;//finally must be equal to ri.size()
			for (; idx < ri.size(); ++idx) {
				let& src = ri[idx].mg;
				assert(src->is(ims_val_b::ETP::matrix));
				auto& XY = maps[idx * 3];
				auto& YZ = maps[idx * 3 + 1];
				auto& XZ = maps[idx * 3 + 2];

				A = qm * src.get()->MatR() * rm;
				bt = qm * src.get()->TrR();
				//s0 | s1 | k
				//c  | d  | g
				//s2 | s3 | q

				Real s0 = A(0, 0);
				Real s1 = A(0, 1);
				Real s2 = A(2, 0);
				Real s3 = A(2, 1);

				Real dt = s0 * s3 - s1 * s2;

				Real i = 1;
				Real f = 1;

				Real c = A(1, 0) / f;
				Real d = A(1, 1) / f;
				Real k = A(0, 2) / i;
				Real q = A(2, 2) / i;
				Real g = A(1, 2);
				let dv = ((s0 * d * q + s3 * c * k) - (s1 * c * q + s2 * d * k));
				if (std::abs(dv) < eps) {
					break;//keep looking
				}
				Real h = dt / dv;
				Real a = 1;
				Real j = (s0 - c * h * k);
				Real p = (s2 - c * h * q);

				if (std::abs(j) < eps) {
					break;//keep looking
				}
				Real b = (s1 - d * h * k) / j;

				XY = { a, c, b, d, bt(0), bt(1) };
				YZ = { f, h, g, i, 0, bt(2) };
				XZ = { j, p, k, q, 0, 0 };

#ifndef NDEBUG
				DynMat<Real> T, MXY(3, 3), MYZ(3, 3), MXZ(3, 3);
				MXY <<
					XY[0], XY[2], 0,
					XY[1], XY[3], 0,
					0, 0, 1;
				MYZ <<
					1, 0, 0,
					0, YZ[0], YZ[2],
					0, YZ[1], YZ[3];

				MXZ <<
					XZ[0], 0, XZ[2],
					0, 1, 0,
					XZ[1], 0, XZ[3];

				T = MXZ * MYZ * MXY - A;
				assert(T.norm() < eps);
#endif //NDEBUG

			}
			if (idx == ri.size()) {
				break;//managed to convert every element
			}
		}

	}

	size_t width = 800;
	size_t height = 480;

	str << "<flame ";
	str << "name =\"" << name << "\" ";
	str << "size =\"" << width << " " << height << "\" ";

	str << "brightness = \"0.5\" gamma = \"4\" ";

	str << "version =\"" << APPLICATION_TITLE " v" PROJECT_VERSION << "\" ";

	if (dim == 2) {
		str << "center =\"" << sd.c[0] << " " << -sd.c[1] << "\" ";
		str << "scale =\"" << std::min(width, height) / sd.r / 2 << "\" ";
	} else {
		str << "center =\"" <<
			0 << " " <<
			0 << "\" ";
		let zoom = 4;
		str << "cam_zoom =\"" << zoom << "\" ";

		str << "cam_pos_x =\"" << 0 << "\" ";
		str << "cam_pos_y =\"" << 0 << "\" ";
		str << "cam_pos_z =\"" << 0 << "\" ";
		str << "preserve_z =\"1\" ";

		//scale = "56.25"

	}

	str << "background =\"" << background[0] << " " << background[1] << " " << background[2] << "\" ";
	str << ">\r\n";

	auto clamp = [](Real x)->Real
	{
		if (std::abs(x) < 10 * ims_num_traits<Real>::epsilon())return 0;
		return x;
	};

	//let pal_size = pal.data.size();
	size_t pal_size = 256;

	//number of vertices
	size_t nv = 0;
	for (let& fe : fedegs) {
		nv = std::max(nv, fe.vs + 1);
		nv = std::max(nv, fe.vt + 1);
	}

	//each edge has its own map (duplicates are possible)

	size_t num_edges = fedegs.size();

	std::vector<uint8_t> chaos;
	chaos.resize(nv * num_edges, 0);


	for (size_t idx = 0; idx < fedegs.size(); ++idx) {
		let& fe = fedegs[idx];
		chaos[fe.vt * num_edges + idx] = 1;
	}

	static const char* id_str = "1 0 0 1 0 0";
	for (size_t idx = 0; idx < fedegs.size(); ++idx) {
		let& fe = fedegs[idx];

		let vs = fe.vs;

		str << "<xform ";

		static const std::array < const char*, 3 > KW =
		{ "coefs","yzCoefs","zxCoefs" };

		if (dim == 2) {
			str << KW[0] << " =\"";
			if (fe.m == ims_max) {
				str << id_str;
			} else {
				let* m = ri[fe.m].mg.get();
				std::array<double, 6> a;
				if (m->is(ims_val_b::ETP::number)) {
					a = { m->get_real(),0,0,m->get_real(), 0,0 };
				} else {
					a = {
						m->MatR()(0, 0), -m->MatR()(1, 0),
						-m->MatR()(0, 1), m->MatR()(1, 1),
						m->TrR()(0), -m->TrR()(1)
					};
				}
				for (auto& q : a) {
					q = clamp(q);
				}
				str << a[0] << " " << a[1] << " "
					<< a[2] << " " << a[3] << " "
					<< a[4] << " " << a[5];
			}
			str << "\" ";
		} else {

			for (size_t ii = 0; ii < KW.size(); ++ii) {
				str << KW[ii] << " =\"";
				if (fe.m == ims_max) {
					str << id_str;
				} else {
					let& m = maps[fe.m * 3 + ii];
					str << clamp(m[0]) << " " << clamp(m[1]) << " "
						<< clamp(m[2]) << " " << clamp(m[3]) << " "
						<< clamp(m[4]) << " " << clamp(m[5]);
				}

				str << "\" ";
			}
		}

		str << "weight =\"" << fe.w << "\" ";

		let clr = float(pal.adjust(fe.clr_idx)) / (pal_size - 1);
		str << "color =\"" << clr << "\" ";

		str << "symmetry =\"-1\" ";
		if (fe.m != ims_max) {
			str << "linear =\"1\" ";
		}


		if (nv > 1) {
			str << "chaos =\"";

			for (size_t i = 0; i < num_edges; ++i) {
				str << int(chaos[vs * num_edges + i]) << " ";
			}
			str << "\" ";

			let opa = (fe.m != ims_max && fe.vs == ver ? 1 : 0);
			str << "opacity =\"" << opa << "\" ";
		}

		str << "/>\r\n";

	}

	str << "<palette count = \"" << pal_size << "\" format=\"RGB\">\r\n";
	pal.save_hex_rgb(str, pal_size);
	str << "\r\n</palette>\r\n";

	str << "</flame>";
}


void ims_to_fractracer(
	std::ostream& str,
	size_t n,
	const ims_graph_base& gm,
	const camera<Real>& cam,
	size_t ver,
	std::span<const edge_map> ri,
	const palette& pal)
{

	assert(n == 2 || n == 3);

	ims_precision prec(str);
	prec.template max<Real>();

	str << "return" << nlc;
	str << "--<auto>" << nlc;
	str << "{" << nlc;

	////////////////////////////////////////////////////////////////////////////
	str << "objects=" << nlc;
	str << "{" << nlc;

	str << "{ '*', { 'camera1','light_source1','light_ambient1','root" << "'} }," << nlc;

	let num_ver = gm.num_ver();

	///////////////////////////////////////////////////////////////
	//root

	let root_edges = gm.num_edges(ver);

	str << "root" << "={ '+',{";
	for (size_t j = 0; j < root_edges; ++j) {
		str << "'e" << ver << "_" << j << "',";
	}
	str << "} }," << nlc;

	for (size_t j = 0; j < root_edges; ++j) {
		str << "e" << ver << "_" << j << "={ '*',{";
		str << "'c" << j << "',";
		str << "'d" << ver << "_" << j << "',";
		str << "} }," << nlc;
	}

	for (size_t j = 0; j < root_edges; ++j) {
		str << "c" << j << "={'color'}," << nlc;
	};
	///////////////////////////////////////////////////////////////

	for (size_t i = 0; i < num_ver; ++i) {
		let ne = gm.num_edges(i);
		str << "s" << i << "={ '+',{";
		for (size_t j = 0; j < ne; ++j) {
			str << "'d" << i << "_" << j << "',";
		}
		str << "} }," << nlc;
	}

	for (size_t i = 0; i < num_ver; ++i) {
		let ne = gm.num_edges(i);
		for (size_t j = 0; j < ne; ++j) {
			let& e = gm.get_edge(i, j);
			str << "d" << i << "_" << j << "={ '*',{";
			str << "'m" << e.m << "'," << "'s" << e.second << "'";
			str << "} }," << nlc;
		}
	}

	for (size_t i = 0; i < ri.size(); ++i) {
		str << "m" << i;
		if (n == 2) {
			str << "={'matrix2d'}," << nlc;
		} else {
			str << "={'matrix3d'}," << nlc;
		}

	}



	///////////////////////////////////////////////////////////////



	str << "camera1 = { 'camera' }," << nlc;
	str << "light_ambient1 = { 'light_ambient' }," << nlc;
	str << "light_source1 = { 'light_source' }," << nlc;

	str << "}," << nlc;
	////////////////////////////////////////////////////////////////////////////

	str << "cparams =" << nlc;
	str << "{" << nlc;


	str << "camera1 = {" << nlc;
	str << "	fov = " << cam.m_fov << "," << nlc;
	str << "	location = { " << cam.m_loc(0) << "," << cam.m_loc(1) << "," << cam.m_loc(2) << " }," << nlc;
	str << "	look_at = { " << cam.m_ref(0) << "," << cam.m_ref(1) << "," << cam.m_ref(2) << " }," << nlc;
	str << "	sky = { " << cam.m_ver(0) << "," << cam.m_ver(1) << "," << cam.m_ver(2) << " }," << nlc;
	str << "}," << nlc;
	str << "light_ambient1 = { 1,1,1,0.25, }," << nlc;
	str << "light_source1 = {" << nlc;
	str << "	color = { 1,1,1,0.15, }," << nlc;
	str << "	location = { " <<
		cam.m_loc(0) << ", " <<
		cam.m_loc(1) << ", " <<
		cam.m_loc(2) << " }," << nlc;
	str << "}," << nlc;

	for (size_t i = 0; i < ri.size(); ++i) {
		str << "m" << i << "={";
		let* m = ri[i].mg.get();
		let is_num = m->is(ims_val_b::ETP::number);
		assert(is_num || m->is(ims_val_b::ETP::matrix));
		assert(m->is(ims_val_b::EST::real));
		for (size_t r = 0; r < n; ++r) {
			for (size_t c = 0; c < n; ++c) {
				let v = is_num ? (r == c ? m->get_real() : 0) : m->MatR()(r, c);
				str << v << ",";
			}
		};
		for (size_t r = 0; r < n; ++r) {
			let v = is_num ? 0.0 : m->TrR()(r);
			str << v;
			if (r + 1 < n)str << ",";
		}
		str << "}," << nlc;
	};

	for (size_t j = 0; j < root_edges; ++j) {
		let& clr = pal.get(j).c;
		fmt::print(str, "c{}={{{:.4g},{:.4g},{:.4g}}}," nlc, j, clr[0], clr[1], clr[2]);
	};

	str << "}," << nlc;
	////////////////////////////////////////////////////////////////////////////
	str << "version = { 1,4,2,2 }," << nlc;
	str << "}" << nlc;
	str << "--</auto>" << nlc;
}
