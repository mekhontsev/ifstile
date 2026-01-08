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
#include <numbers>


#include "math_helpers.h"
#include "ims_file.h"
#include "ims_graph.h"
#include "block_form.h"
#include "ims_pool.h"
#include "ims_graph_base.h"
#include "dfs.h"
#include "matrix_funcs.h"
#include "poly_roots.h"

template<typename Number>
void test_eigen() 
{
	Eigen::Matrix<Number, Eigen::Dynamic, Eigen::Dynamic> A;
	A.resize(2, 2);
	A << 0, -1, 1, 0;

	intptr_t r, c;
	EXPECT_EQ(A.maxCoeff(&r, &c), 1);
	EXPECT_EQ(r, 1);
	EXPECT_EQ(c, 0);
	EXPECT_EQ(A.partialPivLu().determinant(), 1);
	EXPECT_EQ(A.determinant(), 1);
	EXPECT_EQ(A.fullPivLu().determinant(), 1);

}

TEST(testEigen, eigen_double)
{
	test_eigen<double>();
}
TEST(testEigen, eigen_big_float)
{
	test_eigen<float_number_boost>();
}

////////////////////////////////////////////////////////////////////////////////

TEST(testMath, matrix_mul1)
{
	using r_small = boost::rational<int64_t>;
	//using r_big = boost::multiprecision::cpp_rational;
	
	DynMat<r_small> L{ 2, 3 }, D{ 2, 3 };
	//DynMat<r_big> LB{ 2, 3 }, DB{ 2, 3 };
	
	L << r_small{1, 2}, r_small{ 1 , 3 }, r_small{ 1 , 5 }, r_small{ 1 , 7 }, r_small{ 1 , 11 }, r_small{ 1 , 13 };
	//LB = L.cast<r_big>();

	//let res1 = mul_affine_rational_checked(D.data(), L.data(), L.data(), 2);	L = D;
	//EXPECT_TRUE(res1);

	//let res2 = mul_affine_rational_checked(D.data(), L.data(), L.data(), 2);	L = D;
	//EXPECT_TRUE(res2);

	//let res3 = mul_affine_rational_checked(D.data(), L.data(), L.data(), 2);	L = D;
	//EXPECT_FALSE(res3);

}


TEST(testImsPool, test_get_idx)
{
	EXPECT_EQ(ims_pool::get_idx(1), 0);
	EXPECT_EQ(ims_pool::get_idx(8), 0);
	EXPECT_EQ(ims_pool::get_idx(9), 1);
	EXPECT_EQ(ims_pool::get_idx(16), 1);
	EXPECT_EQ(ims_pool::get_idx(17), 2);
	EXPECT_EQ(ims_pool::get_idx(32), 2);
	EXPECT_EQ(ims_pool::get_idx(33), 3);
}

TEST(testImsPool, test_get_size)
{
	EXPECT_EQ(ims_pool::get_size(0), 8);
	EXPECT_EQ(ims_pool::get_size(1), 16);
	EXPECT_EQ(ims_pool::get_size(2), 32);
	EXPECT_EQ(ims_pool::get_size(3), 64);
}




TEST(testGraph, dfs1)
{
	ims_graph g;

	g.create_edge(1, 2);
	g.create_edge(2, 0);
	g.create_edge(0, 3);
	g.create_edge(4, 5);
	g.set_vertex_index(0);

	
	std::vector<size_t> res= { 3, 0, 2, 1, 5, 4 };
	size_t idx = 0;

	dfs_po d;
	
	for (size_t v = d.init(g); v < g.num_ver(); v = d.next(g)) {
		EXPECT_EQ(res[idx++], v);
	}	
}



TEST(testMath, poly_factor1)
{	
	std::vector<double> rpoly = {1};//x+1
	std::vector<std::complex<double>> roots;
	poly_roots::compute(roots, rpoly);
	EXPECT_EQ(roots.size(), 1);

	constexpr double eps = 1e-15;
	EXPECT_NEAR(roots[0].real(), -1, eps);
	EXPECT_NEAR(roots[0].imag(),  0, eps);
};

TEST(testMath, poly_factor2)
{
	std::vector<double> rpoly = { 1,0,0,0 };//x^4+1
	std::vector<std::complex<double>> roots;
	poly_roots::compute(roots, rpoly);
	EXPECT_EQ(roots.size(), 4);

	constexpr double eps = 1e-15;
	constexpr double v = 0.70710678118654752440084436210485;

	for (let& q : roots) {
		EXPECT_NEAR(std::abs(q.real()), v, eps);
		EXPECT_NEAR(std::abs(q.imag()), v, eps);
	}
};


TEST(testMath, test_dim2real)
{
	using Integer = boost::multiprecision::cpp_int;
	double res;
	div_to_real(res, Integer(-1), Integer(-2));
	EXPECT_EQ(res, 1.0/2);
}


TEST(testMath, rational_approximation) 
{
	int64_t p, q;
	const int64_t m = 1000;

	EXPECT_TRUE(rational_approximation(std::numbers::pi, p, q, 1e-6, m));
	EXPECT_EQ(p, 355);
	EXPECT_EQ(q, 113);

	EXPECT_TRUE(rational_approximation(1.0 / 3, p, q, 1e-17, m));
	EXPECT_EQ(p, 1);
	EXPECT_EQ(q, 3);

	EXPECT_TRUE(rational_approximation(1.0 / 7, p, q, 1e-17, m));
	EXPECT_EQ(p, 1);
	EXPECT_EQ(q, 7);

	EXPECT_TRUE(rational_approximation(0.0, p, q, 0.0, m));
	EXPECT_EQ(p, 0);
	EXPECT_EQ(q, 1);

	EXPECT_TRUE(rational_approximation(-1.0/9, p, q, 0.0, m));
	EXPECT_EQ(p, -1);
	EXPECT_EQ(q, 9);
}

//check reflections g_creator_group_info
# if 0
TEST(testMath, check_cyc)
{
	DynMat<int64_t> mat_r;
	std::vector<double> cells = {};

	
	for (let& g : creator_state::get_group_info()) {
		get_exchange(mat_r, g.poly.size() - 1);
		cells.clear();
		for (let c : g.subspace)cells.emplace_back(c);
		EXPECT_TRUE(block_form::check_additional_group(mat_r, g.poly, cells));
	}
}
#endif

TEST(testMath, check_number_of_bits)
{
	EXPECT_EQ(number_of_bits(0), 0);
	EXPECT_EQ(number_of_bits(1), 1);
	EXPECT_EQ(number_of_bits(7), 3);
	EXPECT_EQ(number_of_bits(10), 4);
	EXPECT_EQ(number_of_bits(15), 4);
	EXPECT_EQ(number_of_bits(16), 5);
	EXPECT_EQ(number_of_bits(1ull << 10), 11);
	EXPECT_EQ(number_of_bits(uint64_t(1) << 62), 63);
	EXPECT_EQ(number_of_bits(uint64_t(1) << 63), 64);
}



////////////////////////////////////////////////////////////////////////////////
TEST(testStrings, adjust_path1)
{
	EXPECT_EQ("C:/Users/Documents/IFStile/bin/assets/",
		ims_file::adjust("C:\\Users\\Documents\\IFStile\\bin\\assets\\"));
};


TEST(testStrings, adjust_path2)
{
	EXPECT_EQ("C:/Users/Documents/IFStile/bin/assets",
		ims_file::adjust("C:\\Users/Documents\\IFStile\\bin\\assets"));
};
TEST(testStrings, adjust_path3)
{
	EXPECT_EQ("C:/Users/bin/assets/",
		ims_file::adjust("C:\\Users\\Documents\\..\\bin\\assets\\"));
};
TEST(testStrings, adjust_path4)
{
	EXPECT_EQ("C:/",
		ims_file::adjust("C:\\"));
};
TEST(testStrings, adjust_path5)
{
	EXPECT_EQ("../Users/bin/assets/",
		ims_file::adjust("../Users\\Documents\\..\\bin\\assets\\"));
};
