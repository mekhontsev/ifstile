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
#include "operator_ptr.h"
#include "ims_info.h"
#include "eval_context.h"
#include "error_helper.h"
#include "ims_val_b.h"
#include "block_info.h"
#include "graph_init_data_ptr.h"
#include "affine_calc.h"
#include "ast_maps.h"
#include "variator.h"

struct ims_val;

struct aifs_tester
{
	std::unique_ptr<ims_info> nfo;
	block_info bi;
	eval_context ec;
	ast_maps am;
	affine_calc ac;
	std::string aifs;
	std::string err_msg;
	graph_init_data_ptr gid;
	variator_params vp;
	control_values2 cv;
	oper_block inh;
	
	static constexpr ims_val_b::Real eps = 1e-15;

	error_helper::use_buf ehb;//intercept the program output

	aifs_tester() = default;
	aifs_tester(std::string_view v);

	static operator_ptr get_var_ptr(const oper_block& b, std::string_view var_name);

	//returns an error or an empty string
	bool init();
	bool init_ex();

	const ims_val* eval(std::string_view var, bool is_geom = true);

	//compares the variable from the last block
	bool equal(std::string_view var, ims_val_b::Rational val);

	bool approx(std::string_view var, ims_val_b::Real val);

	bool not_finite(std::string_view var);

	template<typename T>
	bool is_arr(std::string_view var, ims_val_b::ETP t,
		const std::initializer_list<T> arr);

	bool approx_vec(std::string_view var,
		const std::initializer_list<ims_val_b::Real> arr);

	bool equal_vec(std::string_view var,
		const std::initializer_list<ims_val_b::Rational> arr);

	bool approx_affine(std::string_view var,
		const std::initializer_list<ims_val_b::Real> arr);

	bool equal_affine(std::string_view var,
		const std::initializer_list<ims_val_b::Rational> arr);

	std::string eval_as_str(std::string_view var, bool is_geom = true);

	const oper_block* get_last_block() const;

	const oper_block* get_block(std::string_view id);

	std::string get_def(std::string_view var);

	const variable& get_var(std::string_view name) const;
	bool is_closed(std::string_view name) const;
};

