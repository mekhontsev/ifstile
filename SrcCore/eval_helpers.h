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
#include "ims_val_b.h"

struct ims_val;

namespace eval_helpers {

using Real = ims_val_b::Real;
using Rational = ims_val_b::Rational;

const ims_val* create_ball(const Real* c, Real r, size_t dim);

//get the image of the ball under the map
//point_mode - we only act on the center, copying the radius
const ims_val* 
mul_ball(const ims_val* map, const ims_val* b, bool point_mode = false);

//A * B^p
const ims_val* mulpow_affine(const ims_val* A, const ims_val* B, uint8_t p);
const ims_val* to_real3(const ims_val* src);

const ims_val* affine_charpoly(const ims_val* src);
const ims_val* affine_inv(const ims_val* src);//src^-1


const ims_val* sum_affine(
	const ims_val* left,
	const ims_val* right,
	size_t dim);

const ims_val* eval_mod(const ims_val* left, const ims_val* right);

//result, if possible - affine, otherwise composition
const ims_val* mulx(
	const ims_val* left,
	const ims_val* right,
	size_t dim
);

//tries to multiply without composition
const ims_val* try_mulx(
	const ims_val* left,
	const ims_val* right,
	size_t dim);

//returns nullptr on failure
const ims_val* to_affine3(const ims_val* src, size_t dim);

//returns nullptr on failure
const ims_val* to_affine_or_scalar(const ims_val* src, size_t dim);

//converts scalars, vectors of numbers and affine
const ims_val* convert_numeric(
	const ims_val* src,
	ims_val_b::ETP t,
	ims_val_b::EST s,
	size_t dim);

////////////////////////////////////////////////////////////////////////////////

void to_affine_buffer(const ims_val* pv, Real* d);
void to_proj_integer(const ims_val* pv, DynMat<int64_t>& dst);

//converts to extended integer matrix
void to_ext_integer(const ims_val* pv, DynMat<int64_t>& dst);

const ims_val* to_ext_real(const ims_val* v, size_t dim);


void affine_real_set_to(const ims_val* dst, Real v);
void affine_int_set_to(const ims_val* dst, Rational v);

//dst = left * right
void affine_mul_rational(const ims_val* dst, const ims_val* left, const ims_val* right);
void affine_mul_real(const ims_val* dst, const ims_val* left, const ims_val* right);


Real norm_adet(const ims_val* m, Real* adet = nullptr);


////////////////////////////////////////////////////////////////////////////////


/*
multiplies graph edges
geom_only - ignores geometrically identity edges (those that don't change the ball)

A is a canonical map
either NOT a composition, or a composition where only the first factor can be a composition, and a canonical one at that.
this models a linked list...

B is an edge map (a special case of a canonical map)
either NOT a composition, or a composition that does not contain nested compositions
*/
const ims_val* edge_mul(const ims_val* A, const ims_val* B, bool geom_only = false);

//attempts to turn A*B into a single map - for example, a product of affine
//if it can't, returns nullptr
//A, B, and the return value cannot be compositions
const ims_val* collapse_compos(const ims_val* A, const ims_val* B);


//converts affine to higher dimension
const ims_val* real_affine_increase_dim(const ims_val* v, size_t dim);


//appends the map B to the edge map
//sets a new sz
//B cannot be a non-empty composition or a rational
void edge_append(const ims_val** A, size_t& sz, const ims_val* B);


//extends the vector with zeros
const ims_val* extend_real_vector_size(const ims_val* v, size_t new_size);

//returns a fixed point in the form of a ball with radius 0
const ims_val* fixed_point(const ims_val* m);

}//namespace eval_helpers
