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


template<typename T>
struct point3t
{	
	T x,y,z;
	
	point3t(){};
	point3t (T px,T py,T pz){set(px,py,pz);}
	template<typename S> point3t(const S* v){set(v);}

	void set(T ax, T ay, T az){x=ax;y=ay;z=az;};

	void clear(){x=y=z=0;};
	bool empty() const {return x==0 && y==0 && z==0;}

	template<typename S>
	void set(const S* v)
	{	x=static_cast<T>(v[0]);
		y=static_cast<T>(v[1]);
		z=static_cast<T>(v[2]);
	}

	template<typename S>
	void to_arr(S*	v) const 
	{	v[0]=static_cast<S>(x);
		v[1]=static_cast<S>(y);
		v[2]=static_cast<S>(z);
	}

	T* data() {return &x;};
	const T* data() const {return &x;};

	bool eq(const point3t& p) const {return x==p.x && y==p.y && z==p.z;};

	point3t& add(const point3t& p){x+=p.x;y+=p.y;z+=p.z;return *this;};
	point3t& sub(const point3t& p){x-=p.x;y-=p.y;z-=p.z;return *this;};
	point3t& mul(T d){x*=d;y*=d;z*=d;return *this;};
	point3t& mulp(const point3t& p){x*=p.x;y*=p.y;z*=p.z;return *this;};

	//cross product
	point3t cross (const point3t& p) const {
		return point3t(y*p.z-z*p.y, z*p.x-x*p.z, x*p.y-y*p.x);
	}

	T dist2(const point3t& p) const {
		return (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)+(p.z-z)*(p.z-z);
	};
	
	T scalar(const point3t& p) const {return x*p.x+y*p.y+z*p.z;};
	T norm2() const {return x*x+y*y+z*z;};
	T dist(const point3t& p) const {return sqrt(dist2(p));};
	T norm() const {return sqrt(norm2());};

	//normalize the vector, returns the old norm
	T normalize(){T ret=norm();if (ret>0)mul(1/ret);return ret;};

	point3t& operator = (T d){set(d,d,d);return *this;}

	point3t& operator += (const point3t& p){return add(p);}
	point3t& operator -= (const point3t& p){return sub(p);}
	point3t& operator *= (T d){return mul(d);}

	point3t	operator +(const point3t& p) const {return point3t(x+p.x, y+p.y, z+p.z);}
	point3t	operator -(const point3t& p) const {return point3t(x-p.x, y-p.y, z-p.z);}
	point3t	operator *(T d) const {return point3t(x*d, y*d, z*d);}

	T	operator *(const point3t& p) const {return scalar(p);}
	point3t	operator ^(const point3t& p) const {return cross(p);}

	friend bool operator == ( const point3t &a, const point3t &b){return a.eq(b);};
	friend bool operator != ( const point3t &a, const point3t &b){return !a.eq(b);};

	//access by coordinate number
	T& operator [] (size_t i){return (&x)[i];}
	const T& operator [] (size_t i) const {return (&x)[i];}

	//takes the square root of each component 
	// //negative components become zero
	void psqrt(){x=(x<=0)?0:sqrt(x);y=(y<=0)?0:sqrt(y);z=(z<=0)?0:sqrt(z);};

	//sort components by DESCENDING
	void sort(const point3t& src){

		if (src.x>src.y){
			if (src.y>src.z){//x>y>z - already sorted
				set(src.x,src.y,src.z);
			}else if (src.x>src.z){//y<=z<=x
				set(src.x,src.z,src.y);
			}else{//y<=x<=z
				set(src.z,src.x,src.y);
			}
		}else{
			if (src.x>src.z){//y>x>z - already sorted
				set(src.y,src.x,src.z);
			}else if (src.y>src.z){//x<=z<=y
				set(src.y,src.z,src.x);
			}else{//x<=y<=z
				set(src.z,src.y,src.x);
			}
		}
	}
	
	//returns the maximum coordinate by absolute value
	T norm_inf() const
	{
		T m=fabs(x),m2=fabs(y);
		if (m2>m)m=m2;
		m2=fabs(z);
		return m2>m?m2:m;
	};


	void set(const T* arr, size_t arr_size )
	{
		if (!arr)arr_size=0;
		switch(arr_size){
		case 0:set(0,0,0);return;
		case 1:set(arr[0],0,0);return;
		case 2:set(arr[0],arr[1],0);return;
		};

		set(arr[0],arr[1],arr[2]);
	}

	//false if infinite or NAN
	bool finit() const {return _finite(x) && _finite(y) && _finite(z);};

	//returns the face number 0-5, and the uv coordinates on it [-1;1]
	unsigned cube_map( T& u, T& v ) const
	{
		point3t a(fabs(x),fabs(y),fabs(z));
		if (a.x >= std::max(a.y, a.z))		{ u = y / a.x; v = z / a.x; return x > 0 ? 0 : 1; };
		if (a.y >= std::max(a.x, a.z))		{ u = x / a.y; v = z / a.y; return y > 0 ? 2 : 3; };
		/*if (a.z >= std::max(a.x, a.y))*/	{ u = x / a.z; v = y / a.z; return z > 0 ? 4 : 5; };
	}

	//returns a point on the face of the unit cube by its cubic coordinates
	void cube_map_inv(T u, T v, unsigned n)
	{
		T w = (n & 1) ? -1 : 1;
		if (n < 2)		set(w, u, v);
		else if (n < 4)	set(u, w, v);
		else			set(u, v, w); 
	}

	//uv at output in [0,1]
	void sphere_map( T& u, T& v ) const
	{
		T m=2*sqrt(x*x+y*y+(z+1)*(z+1));
		u=x/m+0.5;
		v=y/m+0.5;
	}

	//get the normal to the triangle
	static point3t face_normal(const point3t& p1, const point3t& p2, const point3t& p3)
	{
		point3t nr=p1-p3;
		nr=nr.cross(p2-p1);
		nr.normalize();
		return nr;
	};

};

