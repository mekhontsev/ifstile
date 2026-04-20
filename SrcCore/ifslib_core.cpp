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
#include "ifslib_core.h"
#include "ifs_data_text.h"
#include "block_class.h"
#include "info_printer.h"
#include "report_params.h"
#include "custom_block.h"
#include "ast_stack.h"
#include "inter_type.h"
#include "derived_ifs.h"
#include "block_graph.h"

ifslib_core::ifslib_core()
{
	reset_palette();
}

bool ifslib_core::block_valid() const
{
	if (!m_bb) {
		std::cerr << "No block selected. Please call set_block() with a valid block before." << std::endl;
		return false;
	}
	return true;
}

int32_t ifslib_core::graph_root() const
{
	if (!block_valid()) {
		return -1;
	}

	let root = m_bb->get_froot();
	if (root == ims_max) {
		std::cerr << "Root variable is not defined for the selected block." << std::endl;
		return -1;
	}

	return static_cast<int32_t>(root);
}


int32_t ifslib_core::init(const std::string& aifs)
{
	m_nfo = std::make_unique<ims_info>();
	m_bb = nullptr;

	std::istringstream istr(aifs);
	auto nbeg = std::istreambuf_iterator<char>(istr);
	let nend = std::istreambuf_iterator<char>();


	/////////////////////////////////////////////////

	size_t cur_line = 1;

	read_state rs;

	std::string js_src;
	js_from_stream(js_src, cur_line, nbeg, nend);

	m_nfo->m_js_src = std::move(js_src);


	if (!m_nfo->m_js_src.empty()) {
		m_nfo->m_js_filename = "";
		if (!m_nfo->process_js(rs)) {
			return 0;
		}
	}

	for (;;) {//loop through blocks
		if (!aifs_from_stream_ex(m_nfo->m_list, cur_line, rs, nbeg, nend)) {
			return 0;
		}

		if (rs.m_source_num_lines == 0) {
			break;//completed
		};
	};

	if (m_nfo->m_list.empty() && m_nfo->m_js_src.empty()) {
		std::cerr << "No blocks or JS code found in the input." << std::endl;
		return 0;
	}

	if (!m_nfo->link_refs(0)) {
		return 0;
	}

	m_rp.reset_render_params();
	reset_palette();

	return static_cast<int32_t>(m_nfo->m_list.m_blocks.size());
}

std::string_view ifslib_core::get_root_name(int32_t root_idx) const
{
	if (!block_valid()) {
		return {};
	}

	let* c= m_bb->get_class();
	if (root_idx < 0 || root_idx >= static_cast<int32_t>(c->m_refs.size())) {
		std::cerr << "Root index " << root_idx << " is out of range. Valid range is 0 to " << (c->m_refs.size() - 1) << "." << std::endl;
		return {};
	}
	return m_bb->get_class()->get_var_name(static_cast<size_t>(root_idx));
};


int32_t ifslib_core::calc_diams(std::vector<double>& diams,
	int32_t max_queue_size, int32_t max_result_size)
{
	let root = graph_root();
	if (root == -1) {
		return 0;
	}

	let& b = m_bi.m_vb[root];
	if (!b.defined2()) {
		std::cerr << "Root variable is not defined." << std::endl;
		return 0;
	}

	let dim = b.dim();

	geom_input_data d;
	d.gm = &m_bi.get_fg();
	d.ri = m_bi.m_em;
	d.vb = m_bi.m_vb;
	d.eps = ims_num_traits<double>::epsilon();
	d.max_queue_size = max_queue_size;
	d.max_result_size = max_result_size;
	d.root = root;

	m_diam_s.compute(d);
	let& res = m_diam_s.m_result;
	// Layout: [N, a1[0]..a1[dim-1], b1[0]..b1[dim-1], a2[0]..., ...]
	diams.resize(1 + res.size() * 2 * dim);
	diams[0] = static_cast<double>(res.size());
	for (size_t i = 0; i < res.size(); ++i) {
		let& pair = res[i];
		let idx = 1 + i * 2 * dim;
		for (size_t j = 0; j < dim; ++j) {
			diams[idx + j]       = pair[0](j);
			diams[idx + dim + j] = pair[1](j);
		}
	}

	return 1;
};

int32_t ifslib_core::calc_dists(const double* pt, int32_t pt_dim,
	std::vector<double>& dists,
	int32_t max_queue_size, int32_t max_result_size)
{
	let root = graph_root();
	if (root == -1) {
		return 0;
	}

	let& b = m_bi.m_vb[root];
	if (!b.defined2()) {
		std::cerr << "Root variable is not defined." << std::endl;
		return 0;
	}

	let dim = b.dim();
	if ((size_t)pt_dim != dim) {
		std::cerr << "Input point dimension " << pt_dim << " does not match attractor dimension " << dim << "." << std::endl;
		return 0;
	}

	geom_input_data d;
	d.gm = &m_bi.get_fg();
	d.ri = m_bi.m_em;
	d.vb = m_bi.m_vb;
	d.eps = ims_num_traits<double>::epsilon();
	d.max_queue_size = (size_t)max_queue_size;
	d.max_result_size = (size_t)max_result_size;
	d.root = root;

	dist_solver::Vec center(dim);
	for (size_t j = 0; j < dim; ++j){
		center(j) = pt[j];
	}

	m_dist_s.compute(d, center);
	let& res = m_dist_s.m_result;
	// Layout: [N, a1[0]..a1[dim-1], a2[0]..., ...]
	dists.resize(1 + res.size() * dim);
	dists[0] = static_cast<double>(res.size());
	for (size_t i = 0; i < res.size(); ++i) {
		let& pt_i = res[i];
		for (size_t j = 0; j < dim; ++j)
			dists[1 + i * dim + j] = pt_i[0](j);
	}

	return 1;
};


const double* ifslib_core::root_enclosing_ball() const
{
	let root = graph_root();
	if (root == -1) {
		return nullptr;
	}

	auto& b = m_bi.m_vb[root];
	if (!b.defined2()) {
		std::cerr << "Enclosing ball is not defined for the root variable." << std::endl;
		return nullptr;
	}

	return b.radius_data();
};

double ifslib_core::root_hdim()
{
	let root = graph_root();
	if (root == -1) {
		return std::numeric_limits<double>::quiet_NaN();
	}

	let c = m_bi.get_fg().m_ver2com[root];
	let& d = m_bi.m_im.di[c];
	return d.H;
};


double ifslib_core::root_measure()
{
	let root = graph_root();
	if (root == -1) {
		return std::numeric_limits<double>::quiet_NaN();
	}

	let& fg =m_bi.get_fg();
	let c = fg.m_ver2com[root];

	let card = m_bi.m_af_point_calc.m_points_in_comp[c];

	switch (card)
	{
	case cardinality::error:
		return -1;
	case cardinality::empty:
		return 0;
	case cardinality::point:
		return 1;
	case cardinality::finite:
		return 2;
	case cardinality::countable:
	case cardinality::infmes:
		return std::numeric_limits<double>::infinity();
	default:
		break;
	}

	return m_bi.m_im.measure[root];
};

const double* ifslib_core::root_mass_center() const
{
	let root = graph_root();
	if (root == -1) {
		return nullptr;
	}

	let& m = m_bi.m_im.me[root];
	return m.C.data();
};

const double* ifslib_core::root_mass_moments() const
{
	let root = graph_root();
	if (root == -1) {
		return nullptr;
	}

	let& m = m_bi.m_im.me[root];
	return m.I.data();
};

const double* ifslib_core::root_mass_matrix() const
{
	let root = graph_root();
	if (root == -1) {
		return nullptr;
	}

	let& m = m_bi.m_im.me[root];
	return m.Q.data();
};

block_id_t ifslib_core::get_block_idx(std::string_view block_id) const
{
	if (!m_nfo) {
		std::cerr << "No AIFS data loaded. Please call init() with valid AIFS data before searching for a block." << std::endl;
		return block_id_max;
	}

	if (!block_id.empty()) {
		return m_nfo->m_list.m_idf.find_block_id(block_id);
	}

	//find first visible
	for (let i : m_nfo->m_list.m_blocks) {
		auto* pb = m_nfo->m_list.m_id2data[i].b.get();
		if (pb && !pb->m_flags.hidden) {
			return i;
		}
	}

	return block_id_max;
};

int32_t ifslib_core::set_block(int32_t block_idx)
{
	if (!m_nfo) {
		std::cerr << "No AIFS data loaded. Please call init() with valid AIFS data before selecting a block." << std::endl;
		return 0;
	}

	let bid = static_cast<block_id_t>(block_idx);

	if (block_idx < 0 || bid >= m_nfo->m_list.m_id2data.size()) {
		std::cerr << "Block index " << bid << " is out of range. Valid range is 0 to " << (m_nfo->m_list.m_id2data.size() - 1) << "." << std::endl;
		return 0;
	}

	auto* b = m_nfo->m_list.get_block(bid);

	if (!b) {
		std::cerr << "Block with index " << bid << " not found." << std::endl;
		return 0;
	}

	if (!b) {
		std::cerr << "No blocks found." << std::endl;
		return 0;
	}

	if (m_bb != b) {
		m_bi.set_to_recalc_graph();
		if (!m_bi.init4(*b)) {
			std::cerr << "Failed to initialize block info for block: " << bid << std::endl;
			return 0;
		}
		m_sv.clear8();
		m_sv.eval_builtins(*b, m_bi.m_ctx);

		if (!m_bi.compute_metrics()) {
			std::cerr << "Failed to compute block metrics for block: " << bid << std::endl;
			//even if the moment or dimension could not be calculated correctly
			//important values are filled with acceptable values
		}
	}

	m_bb = b;

	//set the root attractor set to the default one
	size_t root_ref = m_sv.eval_root(*b);
	if (root_ref == ims_max) {
		root_ref = b->find_default_ref();
	}
	b->set_active_ref(root_ref);

	m_fit = !m_sv.chas_builtin(builtin_ids::camera);

	if (m_sv.chas_builtin(builtin_ids::colorize)) {
		if (m_sv.m_colorize.is_tiling()){
			m_depth = m_sv.m_colorize.get_depth();
		}
		m_mode = m_sv.m_colorize.type;
		m_need_reset_parameters=true;
	} else if (m_need_reset_parameters) {
		reset_depth();
		reset_mode();
		m_need_reset_parameters = false;
	}

	if (m_sv.chas_builtin(builtin_ids::palette)) {
		m_rp.m_palette = m_sv.m_pal;
	} else if (m_need_reset_parameters) {
		reset_palette();
		m_need_reset_parameters = false;
	}

	m_nb.clear();

	return (int32_t)b->get_class()->m_refs.size();
}

int32_t ifslib_core::set_root(int32_t root_ref)
{
	if (!block_valid()) {
		return -1;
	}

	let root_ref_u = static_cast<size_t>(root_ref);

	let max_ref = m_bb->get_class()->m_refs.size();
	if (root_ref < 0 || root_ref_u >= max_ref) {
		std::cerr << "Root index " << root_ref <<
			" is out of range. Valid range is 0 to " << (max_ref - 1) << "." << std::endl;
		return -1;
	}

	m_bb->set_active_ref(root_ref_u);
	let root = m_bb->get_froot();

	if (root == ims_max) {
		std::cerr << "Root variable '" << root_ref << "' could not be set." << std::endl;
		return -1;
	}

	let vdim = m_bi.m_ver_dim[root];
	if (vdim == ims_max) {
		std::cerr << "Root variable '" << root_ref << "' is empty or invalid." << std::endl;
		return -1;
	}

	m_fit = !m_sv.chas_builtin(builtin_ids::camera);

	return static_cast<int32_t>(vdim);
}


int ifslib_core::get_var_idx(std::string_view var_name) const
{
	if (!block_valid()) {
		return -1;
	}

	let var_ref = m_bb->get_class()->find_var_by_name(var_name);

	if (var_ref == ims_max) {
		std::cerr << "Variable '" << var_name << "' not found in the selected block." << std::endl;
		return -1;
	}

	return static_cast<int>(var_ref);
}

int32_t ifslib_core::information(const char* what)
{
	if (!block_valid()) {
		return 0;
	}

	std::string_view w(what);

	if (w == "Evaluation") {
		print_ifs_eval(*m_bb, m_bi.m_ctx);
		return 1;
	}
	if (w == "NormalMaps") {
		if (m_bb->get_dim() > 0) {
			print_normal_maps(*m_bb, m_bi.m_ctx);
		}
		return 1;
	}
	if (w == "Projection") {
		print_ifs_proj(*m_bb, m_bi);
		return 1;
	}
	if (w == "Dimension") {
		print_dimensions(std::cout, m_bi, true);
		return 1;
	}
	if (w == "Subspaces") {
		print_subspaces(*m_bb, &m_bi);
		return 1;
	}
	if (w == "AST") {
		print_ast(*m_bb, m_bi);
		return 1;
	}
	if (w == "Components") {
		print_components(*m_bb, m_bi);
		return 1;
	}
	return 0;
}


int32_t ifslib_core::custom_ifs(const report_params& rp, bool boundary_mode)
{
	if (0 == m_nb.num_ver()) {
		std::cerr << "#Neighbor graph is empty. Please calculate the neighbor graph before creating a custom block." << std::endl;
		return 0;
	}

	let num_neighbours = m_nb.set_idx_graph(!boundary_mode);
	if (!num_neighbours) {
		std::cerr << "#Neighbor graph is empty." << std::endl;
		return 0;
	}

	std::unique_ptr<oper_block> block_custom;

	let res = create_neghbours(
		m_nfo->m_list.m_idf,
		*block_custom,
		*m_bb,
		m_nb,
		m_bi.get_fg(),
		boundary_mode ? nullptr : &rp,
		nullptr);

	ASSUME(res);

	ast_stack ai;
	if (!ims_info::link_refs_for_block(*m_nfo, block_custom.get(), ai)) {
		std::cerr << "#Failed to link references for the custom block." << std::endl;
		return 0;
	}

	print_ifs_def(*block_custom);
	return 1;
#if 0

	inter_result ires;

	let ret = create_custom_block(
		block_custom,
		ires,
		m_bi,
		*m_bb,
		m_nfo->m_list.m_idf,
		boundary_mode ? ifs_object_type::boundary : ifs_object_type::custom,
		rp);

	std::cout << "#how many intersections were checked = " << ires.m_gcx << std::endl;
	std::cout << "#depth reached = " << ires.m_depth << std::endl;
	std::cout << "#how many bits were used = " << ires.m_bits << std::endl;
	std::cout << "#minimum depth where an exact overlap was found (0: OSC) = " << ires.m_over_depth << std::endl;
	std::cout << "#intersections are fully created = " << ires.m_completed << std::endl;
	std::cout << "#there was a rational overflow = " << ires.m_overflowed << std::endl;
	std::cout << "#the mode in which the calculations were performed = " <<
		(ires.m_mode == intersect_mode::rational ? "rational" :
			ires.m_mode == intersect_mode::big_rational ?
			"big rational" : "floating point") << std::endl;

	if (!ret)
	{
		return 0;
	};

	ast_stack ai;
	ims_info::link_refs_for_block(*m_nfo, block_custom.get(), ai);

	print_ifs_def(*block_custom);
	return 1;
#endif
}



int32_t ifslib_core::calc_neighbor_graph(inter_result& ires, const integer_ims::settings& settings)
{
	if (!block_valid()) {
		return 0;
	}

	ires = m_cs.calc_inter(m_nb, m_bi, settings);
	return 1;
}


int32_t ifslib_core::set_camera(const double* camera_params, int32_t num_params)
{
	if (!block_valid()) {
		return 0;
	}

	if (num_params == 4) {
		m_sv.m_xcam2.m_sd.c[0] = camera_params[0];
		m_sv.m_xcam2.m_sd.c[1] = camera_params[1];
		m_sv.m_xcam2.m_sd.r = camera_params[2];
		m_sv.m_xcam2.m_sd.a = camera_params[3];
		m_sv.m_xcam2.m_2d_empty = false;
		m_fit = false;
		return 1;
	}

	if (num_params == 10) {
		m_sv.m_xcam2.m_camera.m_loc << camera_params[0], camera_params[1], camera_params[2];
		m_sv.m_xcam2.m_camera.m_ref << camera_params[3], camera_params[4], camera_params[5];
		m_sv.m_xcam2.m_camera.m_ver << camera_params[6], camera_params[7], camera_params[8];
		m_sv.m_xcam2.m_camera.m_fov = camera_params[9];
		m_sv.m_xcam2.m_3d_empty = false;
		m_fit = false;
		return 1;
	}

	if (num_params == 0) {
		m_fit = true;
		return 1;
	}

	return 0;
}

int32_t* ifslib_core::get_graph()
{
	if (!block_valid()) {
		return nullptr;
	}

	if (!m_bi.exists()) {
		std::cerr << "Substitution graph is not available for the selected block." << std::endl;
		return nullptr;
	}

	let& fg = m_bi.get_fg();
	let ne = fg.m_edges.size();
	m_ret_int_array.resize(3 + ne * 3);

	auto* r = m_ret_int_array.data();
	r[0] = static_cast<int32_t>(ne);
	r[1] = static_cast<int32_t>(fg.m_vers.size());
	r[2] = static_cast<int32_t>(m_bi.m_em.size());
	r += 3;
	for (size_t i = 0; i < ne; ++i) {
		let& e = fg.m_edges[i];
		r[0] = static_cast<int32_t>(e.first);
		r[1] = static_cast<int32_t>(e.second);
		r[2] = static_cast<int32_t>(e.m);
		r += 3;
	}

	return m_ret_int_array.data();
}

int32_t ifslib_core::get_vertex(int32_t var_idx)
{
	if (!block_valid()) {
		return -1;
	}

	if (!m_bi.exists()) {
		std::cerr << "Substitution graph is not available for the selected block." << std::endl;
		return -1;
	}

	let ret = m_bb->get_graph()->ref2fg(var_idx);
	if (ret == ims_max) {
		return -1;
	}
	return static_cast<int32_t>(ret);
}

int32_t* ifslib_core::get_neighbor_graph()
{
	if (!block_valid()) {
		return nullptr;
	}

	if (m_nb.m_data.empty()) {
		return nullptr;
	};

	let& dig = m_bi.get_fg();
	let dnv = dig.num_ver();
	let num_ver =
		m_nb.get_neighbor_graph(m_neighbour_graph, m_neighbour_maps, dig);

	let esz = m_neighbour_graph.m_edges.size();

	m_ret_int_array.resize(2 + esz * 4);
	auto* d = m_ret_int_array.data();
	let nv_neigh = num_ver - dnv;
	d[0] = static_cast<int32_t>(esz);
	d[1] = static_cast<int32_t>(nv_neigh);
	d += 2;
	for (let& e : m_neighbour_graph.m_edges) {
		*d++ = e.first < nv_neigh ?
			static_cast<int32_t>(e.first):
			-1-static_cast<int32_t>(e.first - nv_neigh);

		*d++ = e.second < nv_neigh ?
			static_cast<int32_t>(e.second):
			-1-static_cast<int32_t>(e.second - nv_neigh);

		let& m = m_neighbour_maps[e.m];

		*d++ = m.r == ims_max ? -1 : static_cast<int32_t>(m.r);
		*d++ = m.f == ims_max ? -1 : static_cast<int32_t>(m.f);
	}
	return m_ret_int_array.data();
}

int32_t ifslib_core::set_palette(const double* colors, int32_t num_colors)
{
	if (num_colors < 0 || num_colors>65535) {
		std::cerr << "Number of colors " << num_colors << " is out of range. Valid range is 0 to 65535." << std::endl;
		return 0;
	}
	if (colors == nullptr || num_colors == 0) {
		reset_palette();
		return 1;
	}
	auto& d = m_rp.m_palette.data;
	d.resize(num_colors);
	for(int32_t i = 0; i < num_colors; ++i) {
		d[i].c[0] = (float)colors[0];
		d[i].c[1] = (float)colors[1];
		d[i].c[2] = (float)colors[2];
		d[i].c[3] = (float)colors[3];
		colors+=4;
	}

	return	1;
}

int32_t ifslib_core::set_parameter(Param param, double value)
{
	switch (param)
	{
	case Param::Mode:
		if (size_t szval = static_cast<size_t>(value);
			szval < colorize_params::EPAR::e_numpar)
		{
			m_mode = static_cast<colorize_params::EPAR>(szval);
			return 1;
		}
		break;
	case Param::Depth:
		if (value >= 0 && value < 1000) {
			m_depth = value;
			return 1;
		}
		break;
	case Param::Quality:
		if (value >= 1 && value <= 16) {
			m_rp.m_quality = static_cast<float>(value);
			return 1;
		}
		break;
	case Param::Thickness:
		if (value >= 1 && value < 1024) {
			m_rp.m_thickness = static_cast<float>(value);
			return 1;
		}
		break;
	default:
		break;
	}

	std::cerr << "Invalid render parameter or value: " <<
		static_cast<int>(param) << " = " << value << "." << std::endl;
	return 0;
}

double ifslib_core::boundary_dim()
{
	if (!block_valid()) {
		return std::numeric_limits<double>::quiet_NaN();
	}

	if (m_nb.m_data.empty()) {
		return -1;
	};

	m_nb.set_idx_graph(true);
	let graph_ok = m_nb.create_boundary(m_bi.get_fg(), m_inters);
	if (!graph_ok) {
		std::cerr << "Failed to create the boundary graph. Please ensure that the block has a valid neighbor graph before calculating the boundary dimension." << std::endl;
		return std::numeric_limits<double>::quiet_NaN();
	}

	m_inters.init();

	m_bi.m_dim_calc.compute_all_dims(
		m_boundary_measure,
		m_inters,
		ims_view(&m_bi.m_em.data()->det_rootn, sizeof(edge_map)));

	double md = -1;
	for (let& q : m_boundary_measure.di) {
		md = std::max(md, q.H);
	}

	return md;
}

int32_t ifslib_core::set_section(const double* params,
	int32_t space_dim, int32_t section_dim)
{
	let root = graph_root();
	if (root == -1) {
		return 0;
	}

	let sp = static_cast<size_t>(space_dim);
	let sc = static_cast<size_t>(section_dim);

	if (sp != m_bi.m_ver_dim[root]) {
		std::cerr << "Space dimension " << space_dim <<
			" does not match attractor ambient dimension "
			<< m_bi.m_ver_dim[root] << "." << std::endl;
		return 0;
	};

	if (sc >= sp || sc < 2 || sc>3) {
		std::cerr << "Section dimension " << sc <<
			" is invalid. Valid values are 2 or 3, and must be less than the space dimension." << std::endl;
		return 0;
	}

	auto& si = m_sv.m_si2;
	si.resize2(sp, sc);

	for (size_t i = 0; i < sp; ++i) {
		si.origin(i) = params[i];
	}
	for (size_t j = 0; j < sc; ++j) {
		let idx = sp + j * sp;
		for (size_t i = 0; i < sp; ++i) {
			si.basis_user(i, j) = params[idx + i];
		}
	}

	m_sv.m_si_empty = false;

	return 1;
}


int32_t ifslib_core::render(ims_bitmap& dst)
{
	let root2 = graph_root();
	if (root2 == -1) {
		return 0;
	}

	let dim_set = m_bi.m_ver_dim[root2];
	if (dim_set == ims_max || dim_set == 0) {
		return false;
	};

	let tx = dst.w();
	let ty = dst.h();
	///////////////////////////////////////////////
	//initialize the subspace
	auto& sv = m_sv;
	auto& si = sv.m_si2;

	if (!sv.m_si_empty && dim_set != si.get_dim_space()) {
		sv.m_si_empty = true;
	};
	if (sv.m_si_empty) {//default section
		sv.m_si_empty = false;
		si.resize2(dim_set, dim_set);
		si.reset();

		if (dim_set > si.get_section_dim()) {
			builder::set_section(si, m_bi.m_im.me[root2]);
		}
	};
	si.init_si();

	let sds = si.get_section_dim();
	///////////////////////////////////////////////

	// automatic position detection
	auto& cc = m_sv.m_xcam2;
	if (sds == 2 || sds == 1) {

		if (cc.empty(2) || m_fit) {
			builder::adjust2d(
				cc,
				m_sv.m_si2,
				m_bi.m_em,
				m_bi.m_vb,
				m_bi.get_fg(),
				root2,
				tx,
				ty,
				m_rp.m_thickness,
				sds == 2);
		}

	} else {//3D

		if (cc.empty(3)) {//default camera
			//use the projection of the center of mass
			ball3d<double> bound;

			projector proj;
			proj.R = si.basis;
			proj.calc_L_ortho();
			bound.c = proj.L * (m_bi.m_im.me[root2].C - si.origin);
			bound.r = 1;//then adjust3d will straighten it out

			cc.m_camera.randomize(bound);
			cc.m_camera.init();
			cc.m_3d_empty = false;
		}

		if (m_fit) {
			builder::adjust3d(
				cc.m_camera,
				tx,
				ty,
				m_rp.m_thickness,
				si,
				m_bi.m_em,
				m_bi.m_vb,
				m_bi.get_fg(),
				root2);
		}
	}
	///////////////////////////////////////////////

	bool has2dstd = false;
	bool has2dext = false;
	bool has3d = false;


	let& rend = m_rp;

	//builder type
	let& crz = sv.chas_builtin(builtin_ids::colorize) ?
		sv.m_colorize : m_rp.m_colorize;

	if (sds == 3) {
		if (cc.empty(3)) {
			return false;
		}
		has3d = true;
	} else {
		if (cc.empty(2)) {
			return false;
		}
		if (crz.is_field()) {
			has2dext = true;
		} else {
			has2dstd = true;
		}
	}

	//coloring depth (for tiling modes)
	let cdp = std::pow(2.0, -m_depth) +
		ims_num_traits<double>::almost_zero();

	//maximum number of pixels on the last iteration
	let max_pix = tx * ty;

	if (has2dstd)m_buf2d_di.m_img.reserve(max_pix);
	if (has3d)m_buf3d_di.m_img.reserve(max_pix);
	if (has2dext)m_buf_ext_di.m_img.reserve(max_pix);

	if (has2dstd) {
		m_builder2d.reserve_memory(max_pix);
	}
	if (has3d) {
		m_builder3d.reserve_memory(max_pix);
	}
	if (has2dext) {
		m_builder_ext.reserve_memory(max_pix, dim_set);
	}

	////////////////////////////////////////////////////////////////

	clear_color(dst);

	let& pal = rend.m_palette;

	let& fog = sv.chas_builtin(builtin_ids::background) ?
		sv.m_bac.get_fog() : rend.m_background.get_fog();

	////////////////////////////////////////////////////////////////
	//building

	if (has2dstd || has3d) {
		m_cm.init_cmaps(
			m_bi.m_style,
			m_bi.get_fg(),
			pal,
			crz.shift,
			m_mode
		);
	}

	m_ss.gm = &m_bi.get_fg();
	m_ss.m_psi = &si;
	m_ss.ri = m_bi.m_em;
	m_ss.vb = m_bi.m_vb;
	m_ss.mes_mul = m_bi.m_im.mes_mul;
	m_ss.icm = &m_cm;
	m_ss.cdpx = cdp;

	if (has2dstd) {

		screen_params<double> sp;
		cc.m_sd.to_params(sp, tx, ty);

		m_builder2d.m_img2.recreate(tx, ty);
		m_builder2d.m_img2.for_each([](auto& q) {q.clear_color(); });

		m_builder2d.calc_buffer(
			m_ss,
			m_rp.m_quality,
			m_rp.m_thickness,
			root2,
			sp);

		m_builder2d.init_draw(m_buf2d_di);

		m_buf2d_di.init(
			rend.m_brightness,
			rend.m_contrast,
			rend.m_border_pow,
			rend.m_inv_mode);

		to_bitmap(dst, m_buf2d_di, 0, 0, tx, ty, 1);

	} else if (has2dext) {

		screen_params<double> sp;
		cc.m_sd.to_params(sp, tx, ty);

		m_builder_ext.prepare(tx, ty, si, sp);

		let power = crz.params.front();

		m_builder_ext.calc_buffer(
			m_bi.m_ver_dim[root2],
			m_ss,
			root2,
			sp.ps / 2,
			m_rp.m_quality * 4,
			power);

		projector proj;
		proj.R = si.basis;
		proj.calc_L_ortho();

		m_builder_ext.init_draw(m_buf_ext_di, proj.L);

		m_buf_ext_di.init(pal, crz);

		to_bitmap(dst, m_buf_ext_di, 0, 0, tx, ty, 1);

	} else if (has3d) {//3D

		m_builder3d.calc_buffer(
			m_ss,
			tx,
			ty,
			cc.m_camera,
			m_rp.m_quality,
			m_rp.m_thickness,
			root2,
			sv.m_light
		);

		////////////////////////////////////////////////////////

		//cannot be interrupted
		m_builder3d.init_draw(
			m_buf3d_di,
			si,
			cc.m_camera);

		m_buf3d_di.calc_ssao(
			rend.m_ssao_rad_perc,
			rend.m_ssao_samples);

		m_buf3d_di.init(
			rend.m_brightness,
			rend.m_border_pow,
			rend.m_ssao_density,
			fog);

		to_bitmap(dst, m_buf3d_di, 0, 0, tx, ty, 1);

	}//3D

	return 1;
}

