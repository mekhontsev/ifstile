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
#include "ims_file.h"
#include "ims_operator.h"
#include "oper_block.h"
#include "block_class.h"
#include "ifs_list.h"


struct out_block
{
	struct mat
	{
		std::string name;
		int pow;
	};
	std::string m_name;
	std::vector<mat> m_m;
	std::array<int64_t, 3> m_t;

	//recursive
	void from_op(const operator_ptr& ptr)
	{
		let& arr = *ptr.a;
		let& op = ptr.h;

		let* g = arr.get_class();


		let ofs = op.get_offset();
		let t = op.tt;

		switch (t) {
		case ETYPE::reference:
		{
			mat m;
			m.name = g->get_var_name(ofs);
			m.pow = 1;
			m_m.push_back(m);
			break;
		}



		case ETYPE::power_imm:
		{
			from_op(arr.get_ptr(ofs));
			if (!m_m.empty()) {
				m_m.back().pow = (int)op.get_pow_exponent_imm();
			}

			break;
		}

		case ETYPE::vector_imm:

		{
			let sz = op.num_args();
			if (sz > m_t.size()) {
				break;
			}
			for (size_t i = 0; i < sz; ++i) {
				if (t == ETYPE::vector_imm) {
					let& v = arr.m_ops[ofs + i];
					m_t[i] = v.i64;
				}
			}
			break;
		}
		case ETYPE::mul:
		{
			let sz = op.num_args();
			for (size_t i = 0; i < sz; ++i) {
				from_op(arr.get_ptr(ofs + i));
			}
			break;
		}
		default:
			break;
		}

	}




};

struct out_oper
{
	std::string name;
	std::vector<out_block>	arr;

	static void from_file(
		std::vector<out_oper>&	arr,
		const ifs_list& lst)
	{
		arr.clear();

		out_oper c;
		out_block dst;

		for (let id : lst.m_blocks) {
			let* b = lst.get_block(id);
			if (b->m_flags.hidden)continue;

	
			let* g = b->get_class();

			c.name = b->m_name;

			auto& data = c.arr;
			data.clear();

			for (let& q : *b) {
				if (q.is_builtin())continue;
				//left side
				dst.m_name = g->get_var_name(q.gr());

				//right side
				dst.m_m.clear();
				dst.from_op(b->get_ptr(q.pos5));
				data.push_back(dst);
			}

			arr.push_back(c);
		}
	}
};
#ifndef NDEBUG

void ims_to_x3d(const std::string& path, const ifs_list& lst)
{
	std::vector<out_oper>	arr;
	out_oper::from_file(arr, lst);

	std::string h1, h2, h3, head;

	if (!ims_file::get_contents(path + "head.txt", head)) {
		return;
	};

	if (!ims_file::get_contents(path + "h1.txt", h1)) {
		return;
	};

	if (!ims_file::get_contents(path + "h2.txt", h2)) {
		return;
	}

	if (!ims_file::get_contents(path + "h3.txt", h3)) {
		return;
	}

	std::string x3d;

	auto get_map = [](const std::string& s)->const std::string&
	{
		static const std::array<std::string, 4> maps =
	{
		"<Transform rotation='0 0 1 1.5707963267948966'>",
		"<Transform rotation='1 0 0 1.5707963267948966'>",
		"<Transform rotation='-1 1 -1 2.0943951023931957'>",
		"<Transform scale='-1 -1 -1'>",
	};
	if (s == "s0")return maps[0];
	if (s == "s1")return maps[1];
	if (s == "s2")return maps[2];
	return maps[3];
	};

	const char* map_end = "</Transform>";


	for (size_t i = 0; i < arr.size(); ++i) {
		const std::string fn = path + std::to_string(i + 1);

		if (!ims_file::get_contents(fn + ".txt", x3d)) {
			continue;
		}

		std::ofstream str;
		ims_file::open(str, fn + ".html");
		if (!str.is_open())continue;

		str << "<html>\r\n";
		str << head;
		str << "<body>\r\n";

		str << h1;

		str << x3d;

		str << h2;

		let& q = arr[i];



		//<!--h1=s2^1*s0^3*[2,-2,-2]-->
		for (size_t k = 0; k < q.arr.size(); ++k) {
			let& h = q.arr[k];

			str << "<!--" << h.m_name << "=";
			for (let& p : h.m_m) {
				str << p.name << "^" << p.pow << "*";
			}
			str << "[" << h.m_t[0] << "," << h.m_t[1] << "," << h.m_t[2] << "]";

			str << "-->\r\n";

			size_t num = 0;
			for (let& p : h.m_m) {

				let& n = get_map(p.name);

				for (size_t j = 0; j < (size_t)p.pow; ++j) {
					str << n << "\r\n";
					++num;
				}

			}

			/////////////////////////////////////////
			str << "<Transform translation = '";
			for (size_t j = 0; j < 3; ++j) {
				str << h.m_t[j] << " ";
			}
			str << "'>\r\n";
			str << "<Shape USE = 'S" << k << "' />\r\n";
			/////////////////////////////////////////

			for (size_t j = 0; j < num + 1; ++j) {
				str << map_end << "\r\n";
			}

		}

		str << h3;

		str << "</body>\r\n";
		str << "</html>\r\n";
	}



};
#endif