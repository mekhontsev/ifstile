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
#include "oper_block.h"
#include "aifs_tester.h"
#include "block_class.h"
#include "block_graph.h"
#include "variable.h"
#include "edge_ball.h"
#include "edge_map.h"

std::string get_con_data(bool err = false);

TEST(testLoad, invalid_block_header_fail)
{
	aifs_tester t(R"(
@%empty
)");

	if (t.init())FAIL() << "should fail";
};

TEST(testLoad, AeqAA_fail)
{
	aifs_tester t(R"(
@
$dim=1
A=A*A
)");

	if (t.init())FAIL() << "should fail";
};

TEST(testLoad, AeqA1ex_fail)
{
	aifs_tester t(R"(
@
$dim=1
B=1
A=A*B
)");

	if (t.init())FAIL() << "should fail";
};

TEST(testEval, condition_wrong_na_fail)
{
	aifs_tester t(R"(
@
a=if(0, 1)
)");

	if (t.init())FAIL() << "should fail";
};

////////////////////////////////////////////////////////////////////////////////


TEST(testLoad, empty_file)
{
	aifs_tester t("");

	if (!t.init())FAIL() << t.err_msg;
};


TEST(testLoad, empty_id)
{
	aifs_tester t(R"(
@empty
)");

	if (!t.init())FAIL() << t.err_msg;
};



TEST(testLoad, AeqA)
{
	aifs_tester t(R"(
@
$dim=1
A=A
)");

	if (!t.init())FAIL() << t.err_msg;
};




TEST(testLoad, AeqA1)
{
	aifs_tester t(R"(
@
$dim=1
A=A*1
)");

	if (!t.init())FAIL() << t.err_msg;
};




TEST(testLoad, AeqA1ex2)
{
	aifs_tester t(R"(
@
$dim=1
B=1
A=A*B^-1
)");

	if (!t.init())FAIL() << t.err_msg;
};

////////////////////////////////////////////////////////////////////////////////
TEST(testEval, pi)
{
	aifs_tester t(R"(
@
a=$pi
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.approx("a", boost::math::constants::pi<ims_val_b::Real>()));
};


TEST(testEval, simple_call)
{
	aifs_tester t(R"(
@func
n=0                                
ret=n

@
f=func(5) 
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("f", 5));
};

TEST(testEval, recursive_vectors)
{
	aifs_tester t(R"(
@
t=[0, $[0]+10, $[1]+15, $[1]+$[2]]
t3=t[3]
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("t3", 35));
};



TEST(testEval, recursive_fib)
{
	aifs_tester t(R"(
@fib
n = 0                                
ret = if(n-1, fib(n-1) + fib(n-2), 1)

@
v0 = fib(8) 
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("v0", 34));
};

TEST(testEval, recursive_factorial)
{
	aifs_tester t(R"(
@fact
n=0                                
ret=if(n, n*fact(n-1), 1)

@
v0=fact(0) 
v1=fact(1) 
v2=fact(2) 
v5=fact(5) 
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("v1", 1));
	EXPECT_TRUE(t.equal("v0", 1));
	EXPECT_TRUE(t.equal("v2", 2));
	EXPECT_TRUE(t.equal("v5", 120));
};

TEST(testEval, fields)
{
	aifs_tester t(R"(
@t2
n=701

@t1
y=t2
z=0

@
b=t1.y.n
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("b", 701));
};

TEST(testEval, fields_cache)
{
	aifs_tester t(R"(
@f1
a=5

@f2
a=11
b=17

@test
x1=f1()
x2=f1()
x3=f2.a
x4=f2.b
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_EQ(t.ec.m_refs5.size(), 7);//no duplicates

	EXPECT_TRUE(t.equal("x1", 5));
	EXPECT_TRUE(t.equal("x2", 5));
	EXPECT_TRUE(t.equal("x3", 11));
	EXPECT_TRUE(t.equal("x4", 17));
};


TEST(testEval, simple)
{
	aifs_tester t(R"(
@
x=0
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("x", 0));
};



TEST(testEval, condition5)
{
	aifs_tester t(R"(
@
x=if(-1)
y=if(7)
z=if(0)
a=if(0, 10, 20)
b=if(1, 10, 20)
c=if(0, 10, 0, 100, 200)
d=if(0, 10, 1, 100, 200)
e=if(1, 10, 0, 100, 200)
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("x", 0));
	EXPECT_TRUE(t.equal("y", 1));
	EXPECT_TRUE(t.equal("z", 0));
	EXPECT_TRUE(t.equal("a", 20));
	EXPECT_TRUE(t.equal("b", 10));
	EXPECT_TRUE(t.equal("c", 200));
	EXPECT_TRUE(t.equal("d", 100));
	EXPECT_TRUE(t.equal("e", 10));
};

TEST(testEval, clamp)
{
	aifs_tester t(R"(
@clamp
x=0
a=0
b=0
ret = if(a-x, a, x-b, b, x)

@
a0=clamp(4, 5, 7)
a1=clamp(6, 5, 7)
a2=clamp(8, 5, 7)
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a0", 5));
	EXPECT_TRUE(t.equal("a1", 6));
	EXPECT_TRUE(t.equal("a2", 7));
};


TEST(testEval, condition)
{
	aifs_tester t(R"(
@
a0=0
a1=1
b=if(a0, 2, 3)
c=if(a1, 4, 5)
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("b", 3));
	EXPECT_TRUE(t.equal("c", 4));
};

TEST(testEval, simpe_index)
{
	aifs_tester t(R"(
@
a=[10, 11, 12, 13][1]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a", 11));
};

TEST(testEval, parent_call)
{
	aifs_tester t(R"(
@base
r=[7,8]

@func1: base
idx=0
ret = r[idx]

@func2: base
ret = r

@
x=func1(1)
w=func1()
z=func2()
y=z[0]
func=func2
t=func()
g=func2()[1]
#h=(func2())[1]
)");

	if (!t.init())FAIL() << t.err_msg;
	
	EXPECT_TRUE(t.equal("x", 8));
	EXPECT_TRUE(t.equal("w", 7));
	EXPECT_TRUE(t.equal_vec("z", { 7,8 }));
	EXPECT_TRUE(t.equal("y", 7));
	EXPECT_EQ(t.eval_as_str("func"), "@func2");
	EXPECT_TRUE(t.equal_vec("t", { 7,8 }));
	EXPECT_TRUE(t.equal("g", 8));
};

TEST(testEval, call_in_parent)
{
	aifs_tester t(R"(
@f
x=0
y=x

@G1
r=0
a=f(r)

@:G1
r=5
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a", 5));
};


TEST(testEval, new_call)
{
	aifs_tester t(R"(
@B
x=5
y=2
z=x*y
t=x-y

@
b1=$new(B, y, 3)
z1=b1.z
t1=b1.t
b2=$new(B, x, 4)
z2=b2.z
t2=b2.t
b3=$new(B, y, 9, x, 8)
z3=b3.z
t3=b3.t
b4=$new(B, 1, 4+5)
z4=b4.z
t4=b4.t
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("z1", 15));
	EXPECT_TRUE(t.equal("t1", 2));
	EXPECT_TRUE(t.equal("z2", 8));
	EXPECT_TRUE(t.equal("t2", 2));
	EXPECT_TRUE(t.equal("z3", 72));
	EXPECT_TRUE(t.equal("t3", -1));
	EXPECT_TRUE(t.equal("z4", 45));
	EXPECT_TRUE(t.equal("t4", -4));
};


TEST(testEval, parent_call_index)
{
	aifs_tester t(R"(
@base
pi=2*asin(1)
q=1/3
a=3*pi/180
r=[1-q*cos(a),-q*sin(a)]
o=[0,0]
R=[o,r, r,o, o,r, r,o, r,o, o,r, r,o, r,o, r,o] # 12 21 12 21 21 12 21 21 21

@recx: base
n=0
ret = if(n, recx(n-1)+R[n-1][0]*q^n*cos(n*a)-R[n-1][1]*q^n*sin(n*a),r[0]*0.5-r[1]*3^.5/2)

@recy: base
n=0
ret = if(n, recy(n-1)+R[n-1][0]*q^n*sin(n*a)+R[n-1][1]*q^n*cos(n*a),r[0]*3^.5/2+r[1]*0.5)

@
x4x = recx(18)
y4y = recy(18)
)");

	if (!t.init())FAIL() << t.err_msg;
	
	EXPECT_TRUE(t.approx("x4x", 0.4482937410865474));
	EXPECT_TRUE(t.approx("y4y", 0.5784481899229401));
};


TEST(testEval, func_and_index)
{
	aifs_tester t(R"(
@fb
C=$e
ret=C[0][0][1]

@
$dim=2
D=[[2,3]]
X=0
Y=0
C=[D, X, Y]
x = fb(C)
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("x", 3));
};

TEST(testEval, charpoly_)
{
	aifs_tester t(R"(
@
$dim=3
v1=$companion([2.3,4.7,0.501])
cv1=$charpoly(v1);
v2=$companion([1/2,3/2,7/3])
cv2=$charpoly(v2);
c=$charpoly([5,6.2,1,7,8,3,9,2,3]);
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.approx_vec("cv1", { 2.3,4.7,0.501 }));
	EXPECT_TRUE(t.equal_vec("cv2", { {1,2},{3,2},{7,3} }));
	EXPECT_TRUE(t.approx_vec("c", { -69.2,20.6,-16 }));
};


TEST(testEval, modules)
{
	aifs_tester t(R"(
@
a=-11%5
b=11%-5
c=11%6%3
d=6.3%4
e=-11.0%5
f=11.0%-5
g =(5/3)%(4/7)
h =7%0
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a", 4));
	EXPECT_TRUE(t.equal("b", -4));
	EXPECT_TRUE(t.equal("c", 2));
	EXPECT_TRUE(t.approx("d", 2.3));
	EXPECT_TRUE(t.approx("e", 4));
	EXPECT_TRUE(t.approx("f", -4));
	EXPECT_TRUE(t.equal("g", {11,21}));
	EXPECT_TRUE(t.equal("h", 7));
};


TEST(testEval, powers)
{
	aifs_tester t(R"(
@
k=-1^2 #-(1^2)=-1 [google]
a=2^-1^2 #2^-(1^2)=1/2 [google]
b=(2^-1)^2 #1/4
c=(2+2)^2  #16
d=(-1)^2   #1
e=(2^2)^3  #64
f=(-e)^2	#4096
g=-a^-a^2 #-1.18...
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("k", -1));
	EXPECT_TRUE(t.equal("a", { 1,2 }));
	EXPECT_TRUE(t.equal("b", { 1, 4 }));
	EXPECT_TRUE(t.equal("c", 16));
	EXPECT_TRUE(t.equal("d", 1));
	EXPECT_TRUE(t.equal("f", 4096));
	EXPECT_TRUE(t.approx("g", -1.189207115002721));
};

TEST(testEval, neg_ipow_int)
{
	aifs_tester t(R"(
@
a8=(1/2)^-2
a9=(1/2)^-1
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("a8", 4));
	EXPECT_TRUE(t.equal("a9", 2));
};


TEST(testEval, neg_ipow_real)
{
	aifs_tester t(R"(
@
a=(2.5)^-2
b=(2.5)^-1
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.approx("a", 0.16));
	EXPECT_TRUE(t.approx("b", 0.4));
};


TEST(testEval, ipow_for_real_translate)
{
	aifs_tester t(R"(
@
$dim=3
a=[0,0,0.5]^-1
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.approx_vec("a", { 0, 0, -0.5 }));
	
};


TEST(testEval, real_powvec)
{
	aifs_tester t(R"(
@
$dim=2
a=[1,2]^(3/2)
b=[1,2,3]^(3/2)
c=[1,2,3,4]^(3/2) #invalid
d=[1,2,3,4,5]^(3/2)
)");

	if (!t.init())FAIL() << t.err_msg;


	EXPECT_TRUE(t.approx_vec("a", { 1.5, 3 }));
	EXPECT_TRUE(t.approx_vec("b", { 1.5, 3, 4.5 }));
	EXPECT_EQ(t.eval_as_str("c"), "invalid");
	EXPECT_TRUE(t.approx_vec("d", { 1.5, 3, 4.5, 6, 7.5 }));
	
	//EXPECT_EQ(t.ehb.get_buf(), "affine real pow: not implemented");
};



TEST(testEval, int_powvec)
{
	aifs_tester t(R"(
@
$dim=2
a=[1,2]^2	#should be ok
b=[1,2,3]^2  #should be ok
c=[1,2,3,4]^2 #should be affine
d=[1,2,3,4,5]^2 #should be ok
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal_vec("a", { 2, 4 }));
	EXPECT_TRUE(t.equal_vec("b", { 2, 4, 6 }));
	EXPECT_TRUE(t.equal_affine("c", { 7, 15, 10, 22, 0, 0 }));
	EXPECT_TRUE(t.equal_vec("d", { 2, 4, 6, 8, 10 }));
};

TEST(testEval, index)
{
	aifs_tester t(R"(
@
s=[[[7]]]
a=s[0][0][0]
n=1
b=s[n-1][n-1][n-1]
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("a", 7));
	EXPECT_TRUE(t.equal("b", 7));
};

TEST(testEval, parser)
{
	aifs_tester t(R"(
@
$dim=2
a=2
b=-a^0.5
c=-2^0.5
d=-2^0.5-2^0.5
e=2^0.5-2^0.5
f=2^0.5--2^0.5
g=-(-2-2)
h=-2-3
i=(-2)^4
j=(-2)^3
k=(-2)^0.5
v=[-1,0]
r=$companion(v)
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal("a", 2));
	EXPECT_TRUE(t.approx("b", -1.414213562373095));
	EXPECT_TRUE(t.approx("c", -1.414213562373095));
	EXPECT_TRUE(t.approx("d", -2.82842712474619));
	EXPECT_TRUE(t.approx("e", 0));
	EXPECT_TRUE(t.approx("f", 2.82842712474619));
	EXPECT_TRUE(t.equal("g", 4));
	EXPECT_TRUE(t.equal("h", -5));
	EXPECT_TRUE(t.equal("i", 16));
	EXPECT_TRUE(t.not_finite("k"));
	EXPECT_TRUE(t.equal_vec("v", {-1,0}));
	EXPECT_TRUE(t.equal_affine("r", { 0, 1, 1, 0,0,0 }));
};

TEST(testEval, sum_mat)
{
	aifs_tester t(R"(
@
$dim=1
s=$companion([1])
a1=s+s
a2=s*2
)");

	if (!t.init())FAIL() << t.err_msg;

	EXPECT_TRUE(t.equal_affine("s", {-1, 0}));
	EXPECT_TRUE(t.equal_affine("a1", { -2, 0 }));
	EXPECT_TRUE(t.equal_affine("a2", { -2, 0 }));
};


TEST(testEval, index_union)
{
	aifs_tester t(R"(
@
$dim=1
A=1/2*(A | [1]*A)
B=1/3*(B | [1]*A)
C=[A,B]
D=C[0]
)");

	if (!t.init())FAIL() << t.err_msg;
	
	let& D = t.get_var("D");
	EXPECT_TRUE(D.ready[0]);
	EXPECT_TRUE(D.ready[1]);
	EXPECT_TRUE(D.dep_from_cycles);
	EXPECT_TRUE(D.dep_from_unions);
	EXPECT_TRUE(!D.is_subs);
	EXPECT_TRUE(t.is_closed("D"));
	
	let& C = t.get_var("C");
	EXPECT_TRUE(C.ready[0]);
	EXPECT_TRUE(C.ready[1]);
	EXPECT_FALSE(C.dep_from_cycles);//geometric!
	EXPECT_FALSE(C.dep_from_unions);
	EXPECT_TRUE(!C.is_subs);
	EXPECT_TRUE(!t.is_closed("C"));
};


TEST(testEval, test_vars)
{
	aifs_tester t(R"(
@
a=$number($integer(0,1))
)");

	if (!t.init())FAIL() << t.err_msg;
	let& a = t.get_var("a");
	EXPECT_TRUE(a.is_var());
};


TEST(testEval, test_vars2)
{
	aifs_tester t(R"(
@
&r=$number($integer(0,1))
a=b
b=r
)");

	if (!t.init())FAIL() << t.err_msg;
	
	let& r = t.get_var("r");
	let& a = t.get_var("a");
	let& b = t.get_var("b");
	
	EXPECT_TRUE(!r.is_var());
	EXPECT_TRUE(r.is_var2);
	EXPECT_TRUE(r.is_subs);
	EXPECT_TRUE(!a.is_var());
	EXPECT_TRUE(b.is_var());
};



TEST(testEval, recursive_index)
{
	aifs_tester t(R"(
@
A=A
B=A[0]
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_EQ(t.eval_as_str("B", false), "@0{A[0]}");
};


TEST(testGraph, num_maps)
{
	aifs_tester t(R"(
@
$dim=1
A=3^-1*([0] | [1])*A
B=3^-1*([0] | [1] | [2])*B
)");

	if (!t.init())FAIL() << t.err_msg;
	let* g = t.get_last_block()->get_graph();
	EXPECT_EQ(g->m_am.m_ixm.m_maps.size(), 3);
	EXPECT_EQ(g->m_g1.num_ver(), 2);
};

////////////////////////////////////////////////////////////////////////////////

TEST(testIFS, call_union)
{
	aifs_tester t(R"(
@
@index_union
$n=index_union
$dim=1
idx=0
A=1/2*(A | [1]*A)
B=1/3*(B | [1]*B)
C=[A,B]
D=C[idx]


@call_union
$n=call_union
$dim=1
v0 = index_union(0)
v1 = index_union(1)
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.bi.exists());

	size_t num_balls = 0;
	for (let& bx : t.bi.m_vb) {
		if (!bx.defined2())continue;
		EXPECT_EQ(bx.dim(), 1);
		EXPECT_GE(bx.radius(), 0.25);
		++num_balls;
	}
	EXPECT_GE(num_balls, 2);
};

TEST(testIFS, graph1)
{
	aifs_tester t(R"(
@
$dim=1
A=3^-1*[0]*A | 3^-1*[1]*A | 3^-1*[2]
B=A*B
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.bi.exists());
};

TEST(testIFS, topo_override)
{
	aifs_tester t(R"(
@dl
$dim=2
v=[-n,0]
n=$number($integer(2,20))
x=1/(n-1)
c=$companion(v)
g=c
h=[(n-x),(2-x)]*-1
$subspace=[c,0,1]
A=g^-1*u*A
u=h

@UN0:dl
u=h|[0,0]|[1,0]|[2,0]|[3,0]|[4,0]|[5,0]

@:UN0
n=7
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.bi.exists());
	EXPECT_EQ(t.bi.get_fg().num_ver(), 1);
	EXPECT_EQ(t.bi.get_fg().m_edges.size(), 7);
	EXPECT_GT(t.bi.m_vb[0].radius(), 0);
};

////////////////////////////////////////////////////////////////////////////////


TEST(testPrintOp, Test1)
{	
	aifs_tester t(R"(
@
a=2
b=(a^-1)^2
c=9%8%7
)");
	
	if (!t.init())FAIL() << t.err_msg;
	
	EXPECT_EQ(t.get_def("b"), "(a^-1)^2");
	EXPECT_TRUE(t.equal("b", { 1, 4 }));
	EXPECT_EQ(t.get_def("c"), "9%8%7");
};

////////////////////////////////////////////////////////////////////////////////
TEST(testArrFuncs, NegativeIndex)
{
	aifs_tester t(R"(
@
a=[1,2,3]
b=a[-1]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("b", 3));
};

TEST(testArrFuncs, ArrSize)
{
	aifs_tester t(R"(
@
a0=[]
a1=[0]
a2=[0,0]
s0=a0()
s1=a1()
s2=a2()
)");

	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("s0", 0));
	EXPECT_TRUE(t.equal("s1", 1));
	EXPECT_TRUE(t.equal("s2", 2));
};

TEST(testArrFuncs, ArrGenSimple)
{
	aifs_tester t(R"(
@
a=[$]
b=[$,$]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a", {}));
	EXPECT_TRUE(t.equal_vec("b", {}));
};

TEST(testArrFuncs, ArrGenSeq)
{
	aifs_tester t(R"(
@
a=[$, if (4-$(), $(), $)]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a", { 0,1,2,3 }));
};

TEST(testArrFuncs, ArrGenSeq2)
{
	aifs_tester t(R"(
@
a=[$, if (4-$(), [$(1)(),$(1)()*$(1)()], $)]
sa=a()
a3=a[3]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a3", { 3,9 }));
	EXPECT_TRUE(t.equal("sa", 4));
};


TEST(testArrFuncs, ArrPar)
{
	aifs_tester t(R"(
@
a=[7,[11,[13,$(0)[0], $(1)[0], $(2)[0]]]]
b=a[1][1]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("b", { 13,13,11,7 }));
};


TEST(testArrFuncs, ArrGenFib)
{
	aifs_tester t(R"(
@
a=[0, 1, $, if (7-$(), $[-1]+$[-2], $)]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a", { 0,1,1,2,3,5,8 }));
};

TEST(testArrFuncs, ArrGenCopy)
{
	aifs_tester t(R"(
@
b=[2,3,5,7,11]
a=[$, if(b() - $(), b[$()], $)]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a", { 2,3,5,7,11 }));
};

TEST(testArrFuncs, ArrFlat)
{
	aifs_tester t(R"(
@
a=[0, 1, [2, [3, [4, 5]]]]
a1=a[]
a2=a1[]
a3=a2[]
a4=a3[]
s0=a()
s1=a1()
s2=a2()
s3=a3()
s4=a4()
a12=a1[2]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("s0", 3));
	EXPECT_TRUE(t.equal("s1", 4));
	EXPECT_TRUE(t.equal("s2", 5));
	EXPECT_TRUE(t.equal("s3", 6));
	EXPECT_TRUE(t.equal("s4", 6));
	EXPECT_TRUE(t.equal("a12", 2));
	EXPECT_TRUE(t.equal_vec("a3", { 0, 1, 2, 3, 4, 5 }));
	EXPECT_TRUE(t.equal_vec("a4", { 0, 1, 2, 3, 4, 5 }));
};

TEST(testArrFuncs, MatrixFlat)
{
	aifs_tester t(R"(
@
a = $companion([5,6,7])[]
a0=a[0]
a1=a[1]
a2=a[2]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("a0", { 0,1,0 }));
	EXPECT_TRUE(t.equal_vec("a1", { 0,0,1 }));
	EXPECT_TRUE(t.equal_vec("a2", { -5,-6,-7 }));
};


TEST(testArrFuncs, ArrSlice)
{
	aifs_tester t(R"(
@
a = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
p1=a[:]
p2=a[:4]
p3=a[4:]
p4=a[2:7]

n1=a[::-1]
n2=a[8:2:-1]
n3=a[:4:-1]
n4=a[7::-1]
n5=a[-2:-6:-1]
n6=a[-1:-11:-1]
n7=a[0:5:-1]
n8=a[2:8:-2]
n9=a[8:2:-2]

b=[1,[2,3], 4]
b0=b[1:b()]
b1=b0[0]
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal_vec("p1", { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
	EXPECT_TRUE(t.equal_vec("p2", { 0, 1, 2, 3 }));
	EXPECT_TRUE(t.equal_vec("p3", { 4, 5, 6, 7, 8, 9 }));
	EXPECT_TRUE(t.equal_vec("p4", { 2, 3, 4, 5, 6 }));

	EXPECT_TRUE(t.equal_vec("n1", { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }));
	EXPECT_TRUE(t.equal_vec("n2", { 8, 7, 6, 5, 4, 3 }));
	EXPECT_TRUE(t.equal_vec("n3", { 9, 8, 7, 6, 5 }));
	EXPECT_TRUE(t.equal_vec("n4", { 7, 6, 5, 4, 3, 2, 1, 0 }));
	EXPECT_TRUE(t.equal_vec("n5", { 8, 7, 6, 5 }));
	EXPECT_TRUE(t.equal_vec("n6", { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }));
	EXPECT_TRUE(t.equal_vec("n7", {}));
	EXPECT_TRUE(t.equal_vec("n8", {}));
	EXPECT_TRUE(t.equal_vec("n9", { 8, 6, 4 }));

	EXPECT_TRUE(t.equal_vec("b1", { 2, 3 }));
};
////////////////////////////////////////////////////////////////////////////////
TEST(testJS, ConsoleLog)
{
	aifs_tester t(R"(
console.log('ConsoleLog')
)");
	get_con_data(false);
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_EQ(get_con_data(false), "ConsoleLog\n");
}

TEST(testJS, JsInitFunc)
{
	aifs_tester t(R"(
export function js_init(){
	this.a=123
}
@@
@
$init = js_init
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a", 123));
}

TEST(testJS, JsInitFunc2)
{
	aifs_tester t(R"(
export function js_init(){
	this.a=125
}
export const $aifs={$init: 'js_init' };
)");
	if (!t.init())FAIL() << t.err_msg;
	EXPECT_TRUE(t.equal("a", 125));
}

TEST(testJS, JsInitFunc3)
{
	aifs_tester t(R"(
function js_init(){
	this.a=125
}
export const $aifs={$init: 'js_init' };
)");
	if (t.init())FAIL() << "should fail, not exported";
	
}
TEST(testJS, JsInitFunc4)
{
	aifs_tester t(R"(
@
$init = js_init
)");
	if (t.init())FAIL() << "should fail, not found in JS";
}

TEST(testJS, JsInitFunc5)
{
	aifs_tester t(R"(
@
$init = 1
)");
	if (t.init())FAIL() << "should fail, not an identifier";
}

TEST(testJS, JsInitFunc6)
{
	aifs_tester t(R"(
export const $aifs={$init: 0 };
)");
	if (t.init())FAIL() << "should fail, not an identifier";
}
