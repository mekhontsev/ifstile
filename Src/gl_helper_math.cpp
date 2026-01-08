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
#include "gl_helper.h"

#if 0
void gl_helper::glFrustum(float* result,
					  float l, float r, float b, float t, float n, float f)
{
	float* m=result;

	m[0]=	2*n/(r-l);	m[1]=	0; 			m[2]=	0; 				m[3]=0;	
	m[4]=	0;			m[5]=	2*n/(t-b);	m[6]=	0; 				m[7]=0; 	
	m[8]=	(r+l)/(r-l);m[9]=	(t+b)/(t-b);m[10]=	-(f+n)/(f-n); 	m[11]=-1;	
	m[12]=	0; 			m[13]=	0;   		m[14]=	-2*f*n/(f-n);  	m[15]=0;

}
void gl_helper::gluLookAt(
	float* result,
	float eyex, float eyey, float eyez,
	float centerx, float centery, float centerz,
	float upx, float upy, float upz)
{
	float x[3],y[3],z[3],d;

	//Make rotation matrix
	//Z vector
	z[0] = eyex - centerx;
	z[1] = eyey - centery;
	z[2] = eyez - centerz;

	d = 1/sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
	z[0] *= d;  z[1] *= d;   z[2] *= d;

	//X vector = up cross Z
	x[0] = upy * z[2] - upz * z[1];
	x[1] =-upx * z[2] + upz * z[0];
	x[2] = upx * z[1] - upy * z[0];

	//Normalize
	d = 1/sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
	x[0] *= d; x[1] *= d; x[2] *= d;

	//Y vector = Z cross X
	y[0] = z[1] * x[2] - z[2] * x[1];
	y[1] =-z[0] * x[2] + z[2] * x[0];
	y[2] = z[0] * x[1] - z[1] * x[0];

	d = 1/sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
	y[0] *= d; y[1] *= d; y[2] *= d;

	float tx=eyex*x[0]+eyey*x[1]+eyez*x[2];
	float ty=eyex*y[0]+eyey*y[1]+eyez*y[2];
	float tz=eyex*z[0]+eyey*z[1]+eyez*z[2];

	float* m=result;
	m[0] = x[0]; m[1] = y[0]; m[2] = z[0];	m[3] = 0; 
	m[4] = x[1]; m[5] = y[1]; m[6] = z[1];	m[7] = 0;   
	m[8] = x[2]; m[9] = y[2]; m[10] = z[2]; m[11] = 0;   
	m[12] = -tx; m[13] = -ty; m[14] = -tz;	m[15] = 1.0;
}

void gl_helper::glOrtho(float* result, float  l,  
	float  r,  float  b,  float  t,  float  n,  float  f)
{
	float* m=result;

	m[0] = 2/(r-l);		m[1] = 0;			m[2] = 0;			m[3]= 0;	
	m[4] =0;			m[5] = 2/(t-b); 	m[6] = 0;			m[7]= 0;	
	m[8] = 0;			m[9] = 0;			m[10]= -2/(f-n);	m[11]= 0;	
	m[12]= -(r+l)/(r-l);m[13]= -(t+b)/(t-b);m[14]= -(f+n)/(f-n);m[15]= 1.0;  
};


void gl_helper::GLMulMatrix(float* D, const float* A, const float* B)
{
	for (unsigned i=0;i<4;++i){
		for (unsigned j=0;j<4;++j){
			D[j+i*4]=
				A[0+i*4]*B[j+0*4]+A[1+i*4]*B[j+1*4]+
				A[2+i*4]*B[j+2*4]+A[3+i*4]*B[j+3*4];
		}
	}
}


void gl_helper::GLMulMatrixVec(float* D, const float* A, const float* V)
{
    D[0]=A[0]*V[0]+A[4]*V[1]+A[8]*V[2]+A[12]*V[3];
    D[1]=A[1]*V[0]+A[5]*V[1]+A[9]*V[2]+A[13]*V[3];
    D[2]=A[2]*V[0]+A[6]*V[1]+A[10]*V[2]+A[14]*V[3];
    D[3]=A[3]*V[0]+A[7]*V[1]+A[11]*V[2]+A[15]*V[3];
}


bool gl_helper::gluInvertMatrix(float* inv, const float* m)
{
	
	inv[0] =m[5] * m[10] * m[15] -		m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +		m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -		m[13] * m[7] * m[10];

	inv[4] =-m[4] * m[10] * m[15] +		m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -		m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +		m[12] * m[7] * m[10];

	inv[8] =m[4] * m[9] * m[15] -		m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +		m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -		m[12] * m[7] * m[9];

	inv[12]=-m[4] * m[9] * m[14] +		m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -		m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +		m[12] * m[6] * m[9];

	inv[1] =-m[1] * m[10] * m[15] +		m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -		m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -		m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +		m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +		m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -		m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -		m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +		m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -		m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +		m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +		m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -		m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -		m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +		m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +	m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -		m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +		m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -		m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -		m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +		m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +	m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -		m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -		m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +		m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -		m[8] * m[2] * m[5];

	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)return false;

	for (int i = 0; i < 16; i++) {
		inv[i] /= det;
	}

	return true;
}

#endif