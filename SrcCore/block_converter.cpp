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
#include "block_converter.h"
#include "block_class.h"
#include "eval_helpers.h"
#include "ims_val.h"
#include "ims_info.h"
#include "variable.h"
#include "ovr_data.h"

void val_recognizer::init(const oper_block* to)
{

	check_block(to);
	in_block = to;

	ctx.set_block(*to);
	let nv = to->num_vars();

	m_map.clear();
	m_vals.clear();

	auto reg = [&](const ims_val* v, size_t ref, size_t idx) 
	{
		if (!v)return;

		let& p = m_vals.emplace_back(eval_helpers::to_affine3(v, to->get_dim()));

		bool ok = false;

		IMS_SCOPE([&] {
			if (!ok) {
				m_vals.pop_back();
			}
		});

		if (!p) {
			return;
		}

		if (!p->is(ims_val_b::EST::rational))return;
		size_t n;
		if (p->is(ims_val_b::ETP::number)) {
			n = 1;
			ok = true;
		} else if (p->is_affine()) {
			n = p->extent(0);
			for (size_t i = 0; i < n; ++i) {
				if (p->p_i()[n * n + i] != 0) {
					return;
				}
			}
			ok = true;
		} else {
			
			return;
		}
		std::span<const Rational> key(p->p_i(), n*n);

		m_map[key] = Val{ ref, idx };
	};

	for (size_t i = 0; i < nv; ++i) {
		let* v = ctx.eval_ref(i);
		reg(v, i, ims_max);
	};

	for (size_t i = 0; i < nv; ++i) {
		let* v = ctx.eval_ref(i);
		if (!v || !v->is(ims_val_b::ETP::vector, ims_val_b::EST::other)) {
			continue;
		}
		for (size_t j = 0; j < v->get_size(); ++j) {
			reg(v->p_v()[j], i, j);
		}
	};
}

bool val_recognizer::recognize(const ims_val* sx)
{	
	pool_ptr src(eval_helpers::to_affine3(sx, in_block->get_dim()));
	if (!src || !src->is(ims_val_b::EST::rational)) {
		return false;
	}

	////////////////////////////////////////////////////////////////////////////
	//prepare the block a little
	block.set_parent(in_block->get_parent());
	block.m_ops.resize(1);

	//trying to recognize the matrix
	Key key{ src->p_i(), src->extent(0) * src->extent(0) };
	auto it = m_map.find(key);
	if (it == m_map.end()) {
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	//check the translate

	pool_ptr invSrc(eval_helpers::affine_inv(src.get()));
	if (!invSrc)return false;
	
	let d = invSrc->extent(0);

	auto st = ESUBTYPE::integer;
	bool zero_translate = true;
	
	for(size_t r = 0; r < d; ++r){
		let& v = invSrc->affine_int_get_elem(r, d);
			
		if(v.denominator() != 1){
			st = ESUBTYPE::rational;
		}
		if(v.numerator() != 0){
			zero_translate = false;
		}
	}

	////////////////////////////////////////////////////////////////////
	let ds = 0;
	let ar = block.add_args(ds, ETYPE::mul, zero_translate ? 1 : 2);

	if (it->second.idx == ims_max) {
		block.m_ops[ar].hdr.set_reference(it->second.ref);
	} else {
		let ax = block.add_args(ar, ETYPE::index_imm, 1);
		block.m_ops[ar].hdr.set_i24(it->second.idx);
		block.m_ops[ax].hdr.set_reference(it->second.ref);
	}

	if(zero_translate){
		return true;
	}

	let pbeg = block.set_vector_ex(ar + 1, ETYPE::vector_imm, st, d);

	for(size_t r = 0; r < d; ++r){
		let v = - invSrc->affine_int_get_elem(r, d);

		auto& op = block.m_ops[pbeg + r];

		if(st == ESUBTYPE::integer){
			op.i64 = v.numerator();
		} else{

			let n32 = static_cast<int32_t>(v.numerator());
			let d32 = static_cast<int32_t>(v.denominator());

			if(v.numerator() != n32 || v.denominator() != d32){
				return false;//TODO: define using division
			}

			op.rt[0] = n32;
			op.rt[1] = d32;
		}
	}

	return true;
}


void block_converter::init(const oper_block& conv, const oper_block* to)
{
	tmp.clear();

	ovr_data opod;
	opod.init(conv);
	opod.merge_from(tmp, conv);

	check_block(&tmp);
	
	assert(tmp.get_class() != conv.get_class());
	conv_refs.clear();

	for (let& q : conv) {
		if(q.is_builtin())continue;
		if (tmp.ctx()->m_refs5[q.gr()].is_subs) {
			continue;//do not transfer to a new element
		}
		conv_refs.emplace_back(q.gr());
	}

	rec.init(to);
	assert(to->get_dim() == tmp.get_dim());
}

bool block_converter::convert(oper_block& dst, const oper_block& sr)
{
	auto* g = tmp.get_class();
	let* dst_g = rec.in_block->get_class();


	ctx.set_block(tmp);
	for (let& q : sr) {
		if (q.is_builtin())continue;
		auto& r = ctx.m_refs5[q.gr()];

		r.c = ast_context{ sr.get_ptr(q.pos5),0 };
	}

	dst.clear();
	dst.set_parent(rec.in_block);
	dst.m_class.reset();

	dst.m_flags = sr.m_flags;
	dst.m_name = sr.m_name;
	dst.m_timestamp = sr.m_timestamp;

	dst.m_dim2 = sr.m_dim2;
	dst.m_flags.ready = false;

	uint32_t pos = 0;

	for(let ref : conv_refs){
		let* v = ctx.eval_ref(ref);
		
		auto id = g->get_var_name(ref);
		if (id.size() > 1 && id.back() == '$') {
			id.remove_suffix(1);
		}

		if (!v) {
			ims_error("convert: var {} cannot be evaluated", id);
			return false;
		}

		let* d = g->m_nfo->m_list.m_idf.find_data(id);
		if (!d) {
			ims_error("convert: var {} is undefined", id);
			return false;//error, there is no such variable at all
		}

		auto it = dst_g->m_unk2var.find(d->unk_id);
		if(it == dst_g->m_unk2var.end()){
			ims_error("convert: var {} not found", id);
			return false;
		}

		let dst_ref = it->second;

		//recognize v, but within dst_g!!!
		if(!rec.recognize(v)) {
			//error, impossible to recognize
			ims_error("convert: var {} is not recognized", id);
			return false;
		}

		//generate bytecode in a new dst block for the dst_ref variable.
		let ds = dst.add_var(pos, dst_ref, ctx.m_refs5[ref].is_subs);
		dst.insert_op_ex(ds, rec.get_ptr(), ctx);
	};

	return true;
}
