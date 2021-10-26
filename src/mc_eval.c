//mc_eval.c - MCalc Evaluator
//Copyright (C) 2021  Ayman Wagih Mohsen
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

//evaluator
#include	"mc_internal.h"
#include	<math.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
static const char file[]=__FILE__;
int			g_result=0,//result index can be anywhere between start & end, relocating token requires modifying metadata
			g_modified=0;//last operation did something (eg: evaluating a single scalar does nothing)

//TODO: print pretty
#if 0
int			print_poly_impl(double *coeff, int size)//coeff is NOT a c vector
{
	int k, printed=0;
	double elem;
	for(k=0;k<size;++k)
//	for(k=size-1;k>=0;--k)
	{
		elem=coeff[k];
		if(elem)
		{
			if(printed)
			{
				if(elem>0)
					printed+=printf(" + ");
				else
				{
					printed+=printf(" - ");
					elem=-elem;
				}
			}
			if(!(elem==1&&k))
				printed+=printf("%g", elem);
			if(elem!=1&&k)
				printed+=printf(" ");
			if(k)
				printed+=printf("x");
			else if(k>1)
				printed+=printf("^%d", k);
		}
	}
	if(!printed)
		printed+=printf("0");
	printf("\n");
	return printed;
}
int			print_poly_comp(double *r, double *i, int size)//NOT c vectors
{
	int k, printed=0;
	double re, im;
	for(k=0;k<size;++k)
//	for(k=size-1;k>=0;--k)
	{
		re=r[k], im=i[k];
		if(re&&im)
		{
			if(printed)
			{
				if(re>0)
					printed+=printf(" + ");
				else
				{
					printed+=printf(" - ");
					re=-re;
				}
			}
			printf("(");
			if(!(re==1&&k))
				printed+=printf("%g", re);
			if(!(re==1&&k))
				printed+=printf("%g", re);
			printf(")");
			if(re!=1&&k)
				printed+=printf(" ");
			if(k)
				printed+=printf("x");
			else if(k>1)
				printed+=printf("^%d", k);
		}
		else if(re)
		{
			if(printed)
			{
				if(re>0)
					printed+=printf(" + ");
				else
				{
					printed+=printf(" - ");
					re=-re;
				}
			}
			if(!(re==1&&k))
				printed+=printf("%g", re);
			if(re!=1&&k)
				printed+=printf(" ");
			if(k)
				printed+=printf("x");
			else if(k>1)
				printed+=printf("^%d", k);
		}
	}
	if(!printed)
		printed+=printf("0");
	printf("\n");
	return printed;
}
void		print_poly		(Object const *data)
{
	int ncoeff=v_size(&data->r);
	if(data->type&1)//complex
		print_poly_comp(data->r, data->i, ncoeff);
	else
		print_poly_impl(data->r, ncoeff);
}
void		print_fraction	(Object const *data)
{
	short console_w, console_h;
	int printed, k;
	printed=print_poly_impl(data->r, data->dx);//num
	if(data->dy!=1||v_at(data->r, data->dx)!=1)
	{
		get_console_size(&console_w, &console_h);
		if(printed>console_w-1)
			printed=console_w-1;
		for(k=0;k<printed;++k)//horizontal line
			printf("-");
		printf("\n");

		print_poly_impl(data->r+data->dy, data->dy);//den
	}
}
void		print_signal	(Object const *data)
{
	int k, size=v_size(&data->r);
	for(k=0;k<size;++k)
		printf("[%d]\t%g\n", k, v_at(data->r, k));
}
void		print_matrix	(Object const *data)
{
	int kx, ky, printed=0, kt;
	for(ky=0;ky<data->dy;++ky)
	{
		for(kx=0;kx<data->dx;++kx)
		{
			if(kx)
				for(kt=22-printed;kt>=0;--kt)
					printf(" ");
			printed=printf("%g", v_at(data->r, data->dx*ky+kx));
		}
		printf("\n");
	}
	printf("\n");
}
#endif

void		print_matrix_debug(double *buf, int w, int h)
{
	int kx, ky;

	printf("\n");
	for(ky=0;ky<h;++ky)
	{
		for(kx=0;kx<w;++kx)
			printf("%4g ", buf[w*ky+kx]);
		printf("\n");
	}
	printf("\n");
}
static int	check_e	(Object const *obj1, Object const *obj2, Token *fn, CompileResult *ret)//checks if objects have same dimensions
{
	if(obj1->dx!=obj2->dx||obj1->dy!=obj2->dy)
	{
		compile_error(fn, "Inconsistent size");
		*ret=C_ERROR;
		return 1;
	}
	return 0;
}
static int	check_mm(Object const *obj1, Object const *obj2, Token *fn, CompileResult *ret)
{
	if(obj1->dx!=obj2->dy)
	{
		compile_error(fn, "Inconsistent size");
		*ret=C_ERROR;
		return 1;
	}
	return 0;
}
static int	extract_int(Object const *src, Token *fn)
{
	double val=v_at(src->r, 0);
	val-=floor(val);
	if(val>0.1&&val<0.9)
		compile_error(fn, "Expected an integer");
	return (int)floor(src->r[0]+0.5);
}
const char*	objtype2str(TokenType objtype)
{
	switch(objtype)
	{
//	case T_VAL:		return "element";
	case T_ID:		return "identifier";
	case T_SCALAR:	return "scalar";
	case T_CSCALAR:	return "complex scalar";
	case T_FRAC:	return "polynomial or a fraction";
	case T_CFRAC:	return "complex polynomial or fraction";
	case T_MATRIX:	return "matrix or vector";
	case T_CMATRIX:	return "complex matrix or vector";
	}
	return "???";
}


void		impl_addbuffers(double *dst, double const *a, double const *b, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=a[k]+b[k];
}
void		impl_subbuffers(double *dst, double const *a, double const *b, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=a[k]-b[k];
}
void		impl_negbuffer(double *dst, double const *a, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=-a[k];
}
void		impl_buf_plus_val(double *dst, double const *buf, double val, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=buf[k]+val;
}
void		impl_val_minus_buf(double *dst, double val, double const *buf, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=val-buf[k];
}
void		impl_buf_mul_val(double *dst, double const *buf, double val, int size)
{
	int k;
	for(k=0;k<size;++k)
		dst[k]=buf[k]*val;
}

void		impl_ref(double *m, short dx, short dy)
{
	double pivot, coeff;
	int mindim=dx<dy?dx:dy, it, ky, kx;
	for(it=0;it<mindim;++it)//iteration
	{
		for(ky=it;ky<dy;++ky)//find pivot
		{
			if(v_at(m, dx*ky+it))
			{
				pivot=v_at(m, dx*ky+it);
				break;
			}
		}
		if(ky<dy)
		{
			if(ky!=it)
				for(kx=0;kx<dx;++kx)//swap rows
					coeff=v_at(m, dx*it+kx), v_at(m, dx*it+kx)=v_at(m, dx*ky+kx), v_at(m, dx*ky+kx)=coeff;
			for(++ky;ky<dy;++ky)//subtract pivot row
			{
				coeff=v_at(m, dx*ky+it)/v_at(m, dx*it+it);
				for(kx=it;kx<dx;++kx)
					v_at(m, dx*ky+kx)-=coeff*v_at(m, dx*it+kx);
			}
		}
	}
}
void		impl_rref(double *m, short dx, short dy)//m is a c_vector
{
	double pivot, coeff;
	int mindim=dx<dy?dx:dy, it, ky, kx;
	for(it=0;it<mindim;++it)//iteration
	{
		for(ky=it;ky<dy;++ky)//find pivot
		{
			pivot=v_at(m, dx*ky+it);
			if(pivot)
				break;
		}
		if(ky<dy)
		{
			if(ky!=it)
				for(kx=0;kx<dx;++kx)//swap rows
					coeff=v_at(m, dx*it+kx), v_at(m, dx*it+kx)=v_at(m, dx*ky+kx), v_at(m, dx*ky+kx)=coeff;
			for(ky=0;ky<dy;++ky)
			{
				if(ky==it)//normalize pivot
				{
					coeff=1/v_at(m, dx*it+it);
					for(kx=it;kx<dx;++kx)
						v_at(m, dx*it+kx)*=coeff;
				}
				else//subtract pivot row from all other rows
				{
					coeff=v_at(m, dx*ky+it)/v_at(m, dx*it+it);
					for(kx=it;kx<dx;++kx)
						v_at(m, dx*ky+kx)-=coeff*v_at(m, dx*it+kx);
				}
			}
		}
	}
}
void		impl_det(Object *dst, Object *mat)
{
	int k, dxplus1=mat->dx+1;
	impl_ref(mat->r, mat->dx, mat->dy);
	for(k=1;k<mat->dx;++k)//accumulate diagonal
		v_at(mat->r, 0)*=v_at(mat->r, dxplus1*k);
	v_resize(&dst->r, 1, 0);
	dst->type=T_SCALAR;
	dst->dx=dst->dy=1;
	dst->r[0]=mat->r[0];
}
void		impl_matinv(double *m, short dx, short dy)//resize m to (dy * 2dx),		dx==dy always
{
	int k, size=dx*dy;
			//print_matrix_debug(m, dx<<1, dy);//
	for(k=size-dx;k>=0;k-=dx)//expand M into [M, 0]
	{
		memcpy(m+(k<<1), m+k, dx*sizeof(double));
		memset(m+(k<<1)+dx, 0, dx*sizeof(double));
	}
			//print_matrix_debug(m, dx<<1, dy);//
	for(k=0;k<dx;++k)//add identity: [M, I]
		m[(dx<<1)*k+dx+k]=1;
			//print_matrix_debug(m, dx<<1, dy);//
	impl_rref(m, dx<<1, dy);//[I, M^-1]
			//print_matrix_debug(m, dx<<1, dy);//
	for(k=0;k<size;k+=dx)//pack M^-1
		memcpy(m+k, m+(k<<1)+dx, dx*sizeof(double));
			//print_matrix_debug(m, dx<<1, dy);//
}
void		impl_matmul(double *dst, const double *A, const double *B, int h1, int w1h2, int w2)
{
	int kx, ky, kv;
	double *C;
	for(ky=0;ky<h1;++ky)
	{
		for(kx=0;kx<w2;++kx)
		{
			C=dst+w2*ky+kx;
			for(kv=0;kv<w1h2;++kv)
				*C+=A[w1h2*ky+kv]*B[w2*kv+kx];
		}
	}
}
void		impl_transpose(Object *dst, Object const *mat)
{
	int dx, dy, ky, kx;
	dx=mat->dx;
	dy=mat->dy;
	v_resize(&dst->r, v_size(&mat->r), 0);
	for(ky=0;ky<dx;++ky)
		for(kx=0;kx<dy;++kx)
			v_at(dst->r, dy*ky+kx)=v_at(mat->r, dx*kx+ky);
	dst->type=T_MATRIX;
	dst->dx=dy;//swapped
	dst->dy=dx;
}
void		impl_tensor(Object *dst, Object *A, Object *B)
{
	int dx, dy, kx, ky, kx2, ky2;
	double *m, coeff;

	dx=A->dx*B->dx;
	dy=A->dy*B->dy;
	V_CONSTRUCT(double, m, dx*dy, 0, 0);//SIMD
	for(ky=0;ky<A->dy;++ky)
	{
		for(kx=0;kx<A->dx;++kx)
		{
			coeff=v_at(A->r, A->dx*ky+kx);
			for(ky2=0;ky2<B->dy;++ky2)
				for(kx2=0;kx2<B->dx;++kx2)
					v_at(m, dx*(B->dy*ky+ky2)+B->dx*kx+kx2)=coeff*v_at(B->r, B->dx*ky2+kx2);
		}
	}
	dst->type=T_MATRIX;
	dst->dx=dx;
	dst->dy=dy;
	v_move(&dst->r, &m);
}
void		impl_matdiv(Object *dst, Object *A, Object *B)
{
	int dx, dy, bsize, rsize;
	double *m;

	dx=B->dx, dy=B->dy, bsize=dx*dy;
	V_CONSTRUCT(double, m, bsize<<1, 0, 0);
	memcpy(m, B->r, bsize*sizeof(double));
	impl_matinv(m, dx, dy);
	rsize=A->dy*B->dx;
	v_resize(&m, bsize+rsize, 0);
	memset(m+bsize, 0, rsize*sizeof(double));
	impl_matmul(m+bsize, A->r, m, A->dy, dx, dy);
	dst->type=T_MATRIX;
	dst->dy=A->dy;
	dst->dx=B->dx;
	v_move(&dst->r, &m);
}
void		impl_matdiv_back(Object *dst, Object *A, Object *B)
{
	int asize, rsize, ky, width;
	double *m;

	asize=A->dy*A->dx, rsize=A->dy*B->dx;
	width=A->dx+B->dx;
	V_CONSTRUCT(double, m, asize+rsize, 0, 0);
	for(ky=0;ky<A->dy;++ky)
	{
		memcpy(&v_at(m, width*ky		), &v_at(A->r, A->dx*ky), A->dx*sizeof(double));
		memcpy(&v_at(m, width*ky+A->dx	), &v_at(B->r, B->dx*ky), B->dx*sizeof(double));
	}
	impl_rref(m, width, B->dy);
	for(ky=0;ky<B->dy;++ky)
		mem_shiftback(&v_at(m, B->dx*ky), &v_at(m, width*ky+A->dx), B->dx*sizeof(double));
	v_resize(&m, rsize, 0);
	dst->type=T_MATRIX;
	dst->dy=A->dy;
	dst->dx=B->dx;
	v_move(&dst->r, &m);
}
void		impl_matpow(Object *dst, Object *A, Object *B, Token *fn)
{
	int e, size, dxplus1, k;
	double *cvec, *p, *x, *temp;
//	Object p={T_MATRIX, A->dx, A->dy}, x;

	e=extract_int(B, fn);
	size=A->dx*A->dy;
	V_CONSTRUCT(double, cvec, size*3, 0, 0);
	memset(cvec, 0, size*3*sizeof(double));
	p=cvec, x=p+size, temp=x+size;

	dxplus1=A->dx+1;
	for(k=0;k<size;k+=dxplus1)//identity matrix
		p[k]=1;
	memcpy(x, A->r, size*sizeof(double));
	if(e<0)//negative exponent
		impl_matinv(x, A->dx, A->dy);
	for(;;)
	{
		if(e&1)
		{
			impl_matmul(temp, p, x, A->dx, A->dx, A->dx);
			memcpy(p, temp, size*sizeof(double));
		}
		e>>=1;
		if(!e)
			break;
		impl_matmul(temp, x, x, A->dx, A->dx, A->dx);
		memcpy(x, temp, size*sizeof(double));
	}
	v_resize(&cvec, size, 0);
	dst->type=T_MATRIX;
	dst->dx=dst->dy=A->dx;
	v_move(&dst->r, &cvec);
}

//polynomials are stored as they are read	p[0]*x^(n-1) + p[1]*x^(n-2) + ...p[n-1] of degree n-1
void		impl_polmul(double *res, double const *A, double const *B, int asize, int bsize, int add)//res has correct size of (asize+bsize-1), the arrays are NOT c vectors
{//int add:  -1: subtract from res;  0: overwrite res;  1: add to res
	int dst_size=asize+bsize-1, k, k2;
	double coeff;
	double sign=1;
	if(add==-1)
		sign=-1;
	if(!add)
		memset(res, 0, dst_size*sizeof(double));
	for(k=0;k<asize;++k)//schoolbook O(n2)
	{
		coeff=sign*A[asize-1-k];
		for(k2=0;k2<bsize;++k2)
			res[dst_size-1-k-k2]+=coeff*B[bsize-1-k2];
	}
}
void		impl_poldiv(double const *num, double const *den, int nnum, int nden, double *q, double *r)//qsize=nnum-nden+1, rsize=nnum (actually min(num, nden-1))
{
	int qsize=nnum-nden+1, k, k2;
	memcpy(r, num, nnum*sizeof(double));
	for(k=0;k<qsize;++k)//schoolbook O(n2)
	{
		q[k]=r[k]/den[0];
		for(k2=0;k2<nden;++k2)
			r[k+k2]-=q[k]*den[k2];
	}
}
int			impl_countleadzeros(double *pol, int oldsize)//returns newsize
{
	const double tolerance=1e-10;
	int k;
	for(k=0;k<oldsize;++k)
		if(fabs(pol[k])>tolerance)
			break;
	if(k==oldsize)
		return k-1;
	return k;
}
int			impl_polgcd(double *res, double const *A, double const *B, int asize, int bsize)//res size = min(asize, bsize), returns gcd size
{
	const double tolerance=1e-10;
	double *cvec, *q, *r[3];
	int qsize=maximum(asize, bsize), rsize[3]={asize, bsize, minimum(asize, maximum(bsize-1, 1))}, leadingzeros;
	V_CONSTRUCT(double, cvec, qsize+rsize[0]+rsize[1]+rsize[2], 0, 0);
	q=cvec;
	r[0]=cvec+qsize;
	r[1]=r[0]+rsize[0];
	r[2]=r[1]+rsize[1];
	memcpy(r[0], A, asize*sizeof(double));
	memcpy(r[1], B, bsize*sizeof(double));
	for(;;)
	{
		impl_poldiv(r[0], r[1], rsize[0], rsize[1], q, r[2]);
		leadingzeros=impl_countleadzeros(r[2], rsize[2]);
		r[2]+=leadingzeros, rsize[2]-=leadingzeros;//advance pointer & decrease size, by leading zero count
		if(rsize[2]==1&&fabs(*r[2])<tolerance)
			break;
		q=r[2], r[2]=r[0], r[0]=r[1], r[1]=q, q=cvec;//cycle pointers & their sizes {r[0], <- r[1], <- r[2]}
		leadingzeros=rsize[2], rsize[2]=rsize[0], rsize[0]=rsize[1], rsize[1]=leadingzeros;
	}
	memcpy(res, r[1], rsize[1]*sizeof(double));
	v_destroy(&cvec);
	return rsize[1];
}
int			impl_fracremoveleadzeros(Object *frac)
{
	int numzeros=impl_countleadzeros(frac->r, frac->dx),
		denzeros=impl_countleadzeros(frac->r+frac->dx, frac->dy);
	if(frac->dx-numzeros<=0)
		--numzeros;
	if(numzeros>0)
	{
		mem_shiftback(frac->r, frac->r+numzeros, (frac->dx-numzeros)*sizeof(double));
		frac->dx-=numzeros;
	}
	if(numzeros+denzeros>0)
	{
		mem_shiftback(frac->r+frac->dx, frac->r+frac->dx+numzeros+denzeros, (frac->dy-denzeros)*sizeof(double));
		frac->dy-=denzeros;
		v_resize(&frac->r, frac->dx+frac->dy, 0);
		return 1;
	}
	return 0;
}
void		impl_fracsimplify(Object *frac)
{
	int gcdsize=minimum(frac->dx, frac->dy);
	double *cvec,
		*gcd, *q, *r;
	int k, size, newnumsize, newdensize, resized;
	double inv_a0;

	V_CONSTRUCT(double, cvec, gcdsize+frac->dx+frac->dy, 0, 0);
	gcd=cvec;

	gcdsize=impl_polgcd(gcd, frac->r, &v_at(frac->r, frac->dx), frac->dx, frac->dy);//find gcd(num, den)

	if(gcdsize>1)
	{
		q=cvec+gcdsize;
		newnumsize=frac->dx-gcdsize+1;
		r=q+newnumsize;
		impl_poldiv(frac->r, gcd, frac->dx, gcdsize, q, r);//num/gcd
		memcpy(frac->r, q, newnumsize*sizeof(double));
		
		newdensize=frac->dy-gcdsize+1;
		r=q+newdensize;
		impl_poldiv(frac->r+frac->dx, gcd, frac->dy, gcdsize, q, r);//den/gcd
		memcpy(frac->r+newnumsize, q, newdensize*sizeof(double));

		frac->dx=newnumsize;
		frac->dy=newdensize;
	//	mem_shiftback(frac->r+qsize, frac->r+frac->dx, frac->dy*sizeof(double));//shift back den X
	}
	v_destroy(&cvec);

	resized=impl_fracremoveleadzeros(frac);//resize once
	if(!resized&&gcdsize>1)
		v_resize(&frac->r, frac->dx+frac->dy, 0);
	size=v_size(&frac->r);
	inv_a0=v_at(frac->r, frac->dx);
	if(inv_a0)
	{
		inv_a0=1/inv_a0;
		for(k=0;k<size;++k)
			v_at(frac->r, k)*=inv_a0;
	}
}
void		impl_fracadd(Object *dst, Object *A, Object *B, int subtract)
{
	int n1d2=A->dx+B->dy-1, n2d1=A->dy+B->dx-1;//n1/d1 + n2/d2 = (n1d2 + n2d1)/d1d2
	int rnum, rden;
	double *frac;
	int sign=!subtract-subtract;

	rnum=maximum(n1d2, n2d1);
	rden=A->dy+B->dy-1;
	V_CONSTRUCT(double, frac, rnum+rden, 0, 0);
	if(n1d2>n2d1)
	{
		impl_polmul(		frac,					A->r,		&v_at(	B->r, B->dx),	A->dx, B->dy, 0		);//n1d2 (larger)
		impl_polmul(&v_at(	frac, n1d2-n2d1), &v_at(A->r, A->dx),		B->r,			A->dy, B->dx, sign	);//n2d1
	}
	else
	{
		impl_polmul(&v_at(	frac, n2d1-n1d2),		A->r,		&v_at(	B->r, B->dx),	A->dx, B->dy, 0		);//n1d2
		impl_polmul(		frac,			&v_at(	A->r, A->dx),		B->r,			A->dy, B->dx, sign	);//n2d1 (larger)
	}
	impl_polmul(&v_at(frac, rnum), &v_at(A->r, A->dx), &v_at(B->r, B->dx), A->dy, B->dy, 0);

	dst->type=T_FRAC;
	dst->dx=rnum;
	dst->dy=rden;
	v_move(&dst->r, &frac);
//	impl_fracsimplify(dst);
}
void		impl_fracmul(Object *dst, Object *A, Object *B)
{
	int rnum, rden;
	double *frac;

	rnum=A->dx+B->dx-1;
	rden=A->dy+B->dy-1;
	V_CONSTRUCT(double, frac, rnum+rden, 0, 0);
	impl_polmul(		frac,				A->r,				B->r,			A->dx, B->dx, 0);
	impl_polmul(&v_at(	frac, rnum), &v_at(	A->r, A->dx), &v_at(B->r, B->dx),	A->dy, B->dy, 0);
	
	dst->type=T_FRAC;
	dst->dx=rnum;
	dst->dy=rden;
	v_move(&dst->r, &frac);
//	impl_fracsimplify(dst);
}
void		impl_fracdiv(Object *dst, Object *A, Object *B)
{
	int dx=A->dx+B->dy-1,
		dy=A->dy+B->dx-1;
	double *frac;
	V_CONSTRUCT(double, frac, dx+dy, 0, 0);
	impl_polmul(		frac,				A->r,		&v_at(	B->r, B->dx),	A->dx, B->dy, 0);
	impl_polmul(&v_at(	frac, dx), &v_at(	A->r, A->dx),		B->r,			A->dy, B->dx, 0);

	dst->type=T_FRAC;
	dst->dx=dx;
	dst->dy=dy;
	v_move(&dst->r, &frac);
//	impl_fracsimplify(dst);
}
void		impl_fracpow(Object *dst, Object *A, Object *B, Token *fn)
{
	int e=extract_int(B, fn);
	Object p={T_FRAC, 1, 1}, x;

	V_CONSTRUCT(double, p.r, 2, 0, 0);
	p.r[0]=p.r[1]=1;
	if(e>=0)
		obj_assign(&x, A);
	else//negative exponent
	{
		x.type=A->type;
		x.dx=A->dy;
		x.dy=A->dx;
		V_CONSTRUCT(double, x.r, x.dx+x.dy, 0, 0);
		memcpy(x.r, &v_at(A->r, A->dx), A->dy*sizeof(double));
		memcpy(x.r+x.dx, A->r, A->dx*sizeof(double));
	}
	for(;;)
	{
		if(e&1)
			impl_fracmul(&p, &p, &x);
		e>>=1;
		if(!e)
			break;
		impl_fracmul(&x, &x, &x);
	}
	v_destroy(&x);
	obj_assign(dst, &p);
}
void		impl_ldiv(Object *dst, Object *frac, int nsteps)
{
	int nnum, nden, hsize, k, kh;
	double *num, *den, *q, *h, *h0;
	//int k2;//

	nnum=frac->dx, nden=frac->dy, hsize=maximum(nnum, nden);
	num=frac->r, den=frac->r+nnum;
	V_CONSTRUCT(double, q, nsteps+hsize, 0, 0);//qsize+hsize
	memset(q, 0, (nsteps+hsize)*sizeof(double));
	h=q+nsteps;

	for(k=0;k<nsteps;++k)//standard programming
	{
		h0=h+mod(-k, hsize);
		*h0=k==0;//impulse
		for(kh=1;kh<nden;++kh)
			*h0-=den[kh]*h[mod(kh-k, hsize)];
		for(kh=0;kh<nnum;++kh)
			q[k]+=num[kh]*h[mod(kh-k, hsize)];
		//for(k2=0;k2<nsteps+hsize;++k2)//
		//	printf("  %g", q[k2]);
		//printf("\n");
	}
	v_resize(&q, nsteps, 0);
	dst->type=T_MATRIX;
	dst->dx=1;
	dst->dy=nsteps;
	v_move(&dst->r, &q);
}
void		impl_matpolsubs(double *res, const double *m, double *m_temp, const double *coeff, int ncoeff, int dx)
{
	int size=dx*dx, k, k2;

	//p(x) = (...(c[0]*x + c[1])*x + c[2])...)*x + c[n-1]
	memset(res, 0, size*sizeof(double));
	for(k=0;k<size;k+=dx+1)//res=c[0]*In;
		res[k]=coeff[0];
	for(k=1;k<ncoeff;++k)
	{
		memcpy(m_temp, res, size*sizeof(double));//res*=m
		impl_matmul(res, m_temp, m, dx, dx, dx);

		for(k2=0;k2<size;k2+=dx+1)//res+=c[k]*In
			res[k2]+=coeff[k];
	}
}


static void		unimpl(Token *fn, CompileResult *ret)
{
	compile_error(fn, "Not implemented yet");
	*ret=C_ERROR;
}
static void		r_ans		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int nanswers=v_size(&ans);
	int idx=extract_int(A, fn);
	if(idx<0||idx>=nanswers)
	{
		compile_error(fn, "Out of bounds");
		obj_setscalar(dst, 0, 0);
	}
	else
#ifdef ANS_IDX_ASCENDING
		obj_assign(dst, &v_at(ans, idx));
#else
		obj_assign(dst, &v_at(ans, nanswers-1-idx));
#endif
}
static void		r_roots		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		c_roots		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_ldiv_def	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
//	impl_fracsimplify(A);
	impl_ldiv(dst, A, 20);
}
static void		c_ldiv_def	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_ldiv		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int nsteps=extract_int(B, fn);
//	impl_fracsimplify(A);
	impl_ldiv(dst, A, nsteps);
}
static void		c_ldiv		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_sample	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		c_sample	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_invz		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		c_invz		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_cross3d	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	double *a=A->r, *b=B->r;
	dst->type=T_MATRIX;
	dst->dx=1;
	dst->dy=3;
	v_resize(&dst->r, 3, 0);
	dst->r[0]=a[1]*b[2]-a[2]*b[1];
	dst->r[1]=a[2]*b[0]-a[0]*b[2];
	dst->r[2]=a[0]*b[1]-a[1]*b[0];
}
static void		c_cross3d	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_cross2d	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	double *a=A->r, *b=B->r;
	dst->type=T_SCALAR;
	dst->dx=dst->dy=1;
	v_resize(&dst->r, 1, 0);
	dst->r[0]=a[0]*b[1]-a[1]*b[0];
}
static void		c_cross2d	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_iden		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int size=extract_int(B, fn), nelem=size*size, k;
	dst->type=T_MATRIX;
	dst->dx=dst->dy=size;
	nelem=size*size;
	v_resize(&dst->r, nelem, 0);
	memset(dst->r, 0, nelem*sizeof(double));
	for(k=0;k<nelem;k+=size+1)
		v_at(dst->r, k)=1;
}
static void		r_ref		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_ref(dst->r, dst->dx, dst->dy);
}
static void		c_ref		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_rref		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_rref(dst->r, dst->dx, dst->dy);
}
static void		c_rref		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_det		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_det(dst, A);
}
static void		c_det		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_inv		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	v_resize(&dst->r, v_size(&dst->r)<<1, 0);
	impl_matinv(dst->r, dst->dx, dst->dy);
	v_resize(&dst->r, v_size(&dst->r)>>1, 0);
}
static void		c_inv		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_diag		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}//
static void		c_diag		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_lu		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}//
static void		c_lu		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_trace		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int k, dxplus1=A->dx+1;

	dst->type=T_SCALAR;
	dst->dx=dst->dy=1;
	v_resize(&dst->r, 1, 0);

	v_at(dst->r, 0)=0;
	for(k=0;k<A->dx;++k)
		v_at(dst->r, 0)+=v_at(A->r, dxplus1*k);
}
static void		c_trace		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		c_dft		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		c_idft		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_transpose	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_transpose(dst, A);
}
static void		c_transpose	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_add_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int count=v_size(&A->r);
	v_resize(&dst->r, count, 0);
	impl_addbuffers(dst->r, A->r, B->r, count);
}
static void		c_add_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_add_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_fracadd(dst, A, B, 0);
}
static void		c_add_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		assign		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
}
static void		r_sub_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int count=v_size(&A->r);
	v_resize(&dst->r, count, 0);
	impl_subbuffers(dst->r, A->r, B->r, count);
}
static void		c_sub_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_sub_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_fracadd(dst, A, B, 1);
}
static void		c_sub_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_neg_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int count=v_size(&A->r);
	v_resize(&dst->r, count, 0);
	impl_negbuffer(dst->r, A->r, count);
}
static void		c_neg_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_neg_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_negbuffer(dst->r, dst->r, dst->dx);
}
static void		c_neg_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_add_ew_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, B);
	impl_buf_plus_val(dst->r, dst->r, A->r[0], v_size(&dst->r));
}
static void		c_add_ew_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_add_ew_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_plus_val(dst->r, dst->r, B->r[0], v_size(&dst->r));
}
static void		c_add_ew_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_sub_ew_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, B);
	impl_val_minus_buf(dst->r, A->r[0], dst->r, v_size(&dst->r));
}
static void		c_sub_ew_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_sub_ew_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_plus_val(dst->r, dst->r, -B->r[0], v_size(&dst->r));
}
static void		c_sub_ew_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	if(check_mm(A, B, fn, ret))
		return;
	impl_matmul(dst->r, A->r, B->r, A->dy, A->dx, B->dx);
}
static void		c_mul_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_fracmul(dst, A, B);
}
static void		c_mul_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, B);
	impl_buf_mul_val(dst->r, dst->r, A->r[0], v_size(&dst->r));
}
static void		c_mul_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_mul_val(dst->r, dst->r, B->r[0], v_size(&dst->r));
}
static void		c_mul_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_sf	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, B);
	impl_buf_mul_val(dst->r, dst->r, A->r[0], dst->dx);
}
static void		c_mul_sf	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mul_fs	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_mul_val(dst->r, dst->r, B->r[0], dst->dx);
}
static void		c_mul_fs	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_tensor	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_tensor(dst, A, B);
}
static void		c_tensor	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_s		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_setscalar(dst, A->r[0]/B->r[0], 0);
}
static void		c_div_s		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	if(B->dx!=B->dy)
	{
		compile_error(fn, "Expected a square matrix on right");
	//	compile_error(fn, "Expected a square matrix in denominator");
		*ret=C_ERROR;
		return;
	}
	if(A->dx!=B->dy)
	{
		compile_error(fn, "Left matrix width should be equal to right matrix height");
	//	compile_error(fn, "Expected left operand width same as right operand height");
	//	compile_error(fn, "Inconsistent dimensions for matrix multiplication");
		*ret=C_ERROR;
		return;
	}
	impl_matdiv(dst, A, B);
}
static void		c_div_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_fracdiv(dst, A, B);
}
static void		c_div_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int count=v_size(&dst->r);
	obj_assign(dst, B);
	v_resize(&dst->r, count<<1, 0);
	impl_matinv(dst->r, dst->dx, dst->dy);
	v_resize(&dst->r, count, 0);
	impl_buf_mul_val(dst->r, dst->r, A->r[0], count);
}
static void		c_div_sm	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_mul_val(dst->r, dst->r, B->r[0], v_size(&dst->r));
}
static void		c_div_ms	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_sf	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, B);
	mem_rotate(dst->r, dst->r+dst->dx, dst->r+dst->dx+dst->dy);
	impl_buf_mul_val(dst->r, dst->r, A->r[0], dst->dx);
}
static void		c_div_sf	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_fs	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	obj_assign(dst, A);
	impl_buf_mul_val(dst->r, dst->r, 1/B->r[0], dst->dx);
}
static void		c_div_fs	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_div_back	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	if(A->dx!=A->dy)
	{
		compile_error(fn, "Expected a square matrix on left");
	//	compile_error(fn, "Expected a square matrix in denominator");
		*ret=C_ERROR;
		return;
	}
	if(A->dx!=B->dy)
	{
		compile_error(fn, "Left matrix width should be equal to right matrix height");
		*ret=C_ERROR;
		return;
	}
	impl_matdiv_back(dst, A, B);
}
static void		c_div_back	(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_pow_s		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	dst->r[0]=pow(A->r[0], B->r[0]);
}
static void		c_pow_s		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_pow_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_matpow(dst, A, B, fn);
}
static void		c_pow_m		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_pow_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	impl_fracpow(dst, A, B, fn);
}
static void		c_pow_f		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_mulpolsubs(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int nnum, nden, msize;
	double *cvec, *num, *den, *temp;

	nnum=A->dx, nden=A->dy, msize=B->dx*B->dy;
	V_CONSTRUCT(double, cvec, msize*3, 0, 0);
	num=cvec, den=num+msize, temp=den+msize;

	impl_matpolsubs(num, B->r, temp, A->r, A->dx, B->dx);
	impl_matpolsubs(den, B->r, temp, A->r+A->dx, A->dy, B->dx);
	impl_matinv(den, B->dx, B->dx);
	v_resize(&dst->r, msize, 0);
	impl_matmul(dst->r, num, den, B->dx, B->dx, B->dx);
	v_destroy(&cvec);
}
static void		c_mulpolsubs(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret){unimpl(fn, ret);}
static void		r_cmd		(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret)
{
	int w=extract_int(A, fn), h=extract_int(B, fn);
	v_resize(&dst->r, 1, 0);
	v_at(dst->r, 0)=set_console_buffer_size(w, h);
}
typedef enum//only RC args are promoted
{
	A_ABSENT,
	A_SCALAR_REAL_INT,
	A_SCALAR_RC,
	A_POLY_RC,
	A_FRAC_RC,
	A_VEC3_RC,//cross
	A_VEC2_RC,//cross
	A_MAT_RC,
	A_SQM_RC,//tr, det, inv, diag, lu
	A_POLY_MAT_RC,//dft
	A_ANYOBJ,
} ArgType;
typedef void (*OP_FP)(Object *dst, Object *A, Object *B, Token *fn, CompileResult *ret);
typedef struct//TODO: an overload asks to reorder args
{
	ArgType a1, a2;
//	char a1, a2;
	OP_FP r_func, c_func;//check if c_func!=0
} OpOverload;
typedef struct
{
	TokenType type;
	char dst_is_arg;//op= style assignments
	char ovcount;
	OpOverload *ov;
} OpArgInfo;
static OpOverload
	ov_ans[]={{A_SCALAR_REAL_INT, A_ABSENT, r_ans, 0}},
	ov_cmd[]={{A_SCALAR_REAL_INT, A_SCALAR_REAL_INT, r_cmd, 0}},

	ov_roots[]={{A_POLY_RC, A_ABSENT, r_roots, c_roots}},
	ov_ldiv[]={{A_FRAC_RC, A_ABSENT, r_ldiv_def, c_ldiv_def}, {A_FRAC_RC, A_SCALAR_REAL_INT, r_ldiv, c_ldiv}},
	ov_sample[]={{A_FRAC_RC, A_ABSENT, r_sample, c_sample}},
	ov_invz[]={{A_FRAC_RC, A_ABSENT, r_invz, c_invz}},
	ov_cross[]={{A_VEC3_RC, A_VEC3_RC, r_cross3d, c_cross3d}, {A_VEC3_RC, A_VEC3_RC, r_cross2d, c_cross2d}},
	ov_iden[]={{A_SCALAR_REAL_INT, A_ABSENT, r_iden, 0}},
	ov_ref[]={{A_MAT_RC, A_ABSENT, r_ref, c_ref}},
	ov_rref[]={{A_MAT_RC, A_ABSENT, r_rref, c_rref}},
	ov_det[]={{A_SQM_RC, A_ABSENT, r_det, c_det}},
	ov_inv[]={{A_SQM_RC, A_ABSENT, r_inv, c_inv}},
	ov_diag[]={{A_SQM_RC, A_ABSENT, r_diag, c_diag}},
	ov_lu[]={{A_SQM_RC, A_ABSENT, r_lu, c_lu}},
	ov_trace[]={{A_SQM_RC, A_ABSENT, r_trace, c_trace}},
	ov_dft[]={{A_POLY_MAT_RC, A_ABSENT, 0, c_dft}},//repeat twice
	ov_idft[]={{A_POLY_MAT_RC, A_ABSENT, 0, c_idft}},

	ov_transpose[]={{A_MAT_RC, A_ABSENT, r_transpose, c_transpose}},
	ov_plus[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_add_m, c_add_m},//binary +
		{A_MAT_RC, A_MAT_RC, r_add_m, c_add_m},
		{A_FRAC_RC, A_FRAC_RC, r_add_f, c_add_f},

		{A_SCALAR_RC, A_ABSENT, assign, assign},//unary + (just assigns A)
		{A_MAT_RC, A_ABSENT, assign, assign},
		{A_FRAC_RC, A_ABSENT, assign, assign},
	},
	ov_minus[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_sub_m, c_sub_m},//binary -
		{A_MAT_RC, A_MAT_RC, r_sub_m, c_sub_m},
		{A_FRAC_RC, A_FRAC_RC, r_sub_f, c_sub_f},

		{A_SCALAR_RC, A_ABSENT, r_neg_m, c_neg_m},//unary -
		{A_MAT_RC, A_ABSENT, r_neg_m, c_neg_m},
		{A_FRAC_RC, A_ABSENT, r_neg_f, c_neg_f},
	},
	ov_add_ew[]=
	{
		{A_SCALAR_RC, A_MAT_RC, r_add_ew_sm, c_add_ew_sm},
		{A_MAT_RC, A_SCALAR_RC, r_add_ew_ms, c_add_ew_ms},
	},
	ov_sub_ew[]=
	{
		{A_SCALAR_RC, A_MAT_RC, r_sub_ew_sm, c_sub_ew_sm},
		{A_MAT_RC, A_SCALAR_RC, r_sub_ew_ms, c_sub_ew_ms},
	},
	ov_mul[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_mul_m, c_mul_m},
		{A_MAT_RC, A_MAT_RC, r_mul_m, c_mul_m},
		{A_FRAC_RC, A_FRAC_RC, r_mul_f, c_mul_f},
		{A_SCALAR_RC, A_MAT_RC, r_mul_sm, c_mul_sm},
		{A_MAT_RC, A_SCALAR_RC, r_mul_ms, c_mul_ms},
		{A_SCALAR_RC, A_FRAC_RC, r_mul_sf, c_mul_sf},
		{A_FRAC_RC, A_SCALAR_RC, r_mul_fs, c_mul_fs},
	},
	ov_tensor[]={{A_MAT_RC, A_MAT_RC, r_tensor, c_tensor}},
	ov_div[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_div_s, c_div_s},
		{A_MAT_RC, A_SQM_RC, r_div_m, c_div_m},
		{A_FRAC_RC, A_FRAC_RC, r_div_f, c_div_f},
		{A_SCALAR_RC, A_SQM_RC, r_div_sm, c_div_sm},
		{A_MAT_RC, A_SCALAR_RC, r_div_ms, c_mul_ms},
		{A_SCALAR_RC, A_FRAC_RC, r_div_sf, c_div_sf},
		{A_FRAC_RC, A_SCALAR_RC, r_div_fs, c_div_fs},
	},
	ov_div_back[]={{A_SQM_RC, A_MAT_RC, r_div_back, c_div_back}},
	ov_mod[]={{A_ABSENT, A_ABSENT, 0, 0}},//
	ov_power[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_pow_s, c_pow_s},
		{A_MAT_RC, A_SCALAR_REAL_INT, r_pow_m, c_pow_m},
		{A_FRAC_RC, A_SCALAR_REAL_INT, r_pow_f, c_pow_f},
	},
	ov_assign[]={{A_ANYOBJ, A_ABSENT, assign, assign}},

	//op= style assignment operators
	ov_assign_add[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_add_m, c_add_m},
		{A_MAT_RC, A_MAT_RC, r_add_m, c_add_m},
		{A_FRAC_RC, A_FRAC_RC, r_add_f, c_add_f},
	},
	ov_assign_sub[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_sub_m, c_sub_m},
		{A_MAT_RC, A_MAT_RC, r_sub_m, c_sub_m},
		{A_FRAC_RC, A_FRAC_RC, r_sub_f, c_sub_f},
	},
	ov_assign_mul[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_mul_m, c_mul_m},
		{A_MAT_RC, A_MAT_RC, r_mul_m, c_mul_m},
		{A_FRAC_RC, A_FRAC_RC, r_mul_f, c_mul_f},
	},
	ov_assign_div[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_div_s, c_div_s},
		{A_MAT_RC, A_MAT_RC, r_div_m, c_div_m},
		{A_FRAC_RC, A_FRAC_RC, r_div_f, c_div_f},
	},
	ov_assign_div_back[]={{A_SQM_RC, A_MAT_RC, r_div_back, c_div_back}},
	ov_assign_mod[]={A_ABSENT, A_ABSENT, 0, 0},//
	ov_assign_pow[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_pow_s, c_pow_s},
		{A_SQM_RC, A_SCALAR_REAL_INT, r_pow_m, c_pow_m},
		{A_FRAC_RC, A_SCALAR_REAL_INT, r_pow_f, c_pow_f},
	},
	ov_nothing[]=
	{
		{A_SCALAR_RC, A_SCALAR_RC, r_mul_m, c_mul_m},
		{A_MAT_RC, A_MAT_RC, r_mul_m, c_mul_m},
		{A_FRAC_RC, A_FRAC_RC, r_mul_f, c_mul_f},

		{A_SCALAR_RC, A_MAT_RC, r_mul_sm, c_mul_sm},
		{A_MAT_RC, A_SCALAR_RC, r_mul_ms, c_mul_ms},
		{A_SCALAR_RC, A_FRAC_RC, r_mul_sf, c_mul_sf},
		{A_FRAC_RC, A_SCALAR_RC, r_mul_fs, c_mul_fs},

		{A_FRAC_RC, A_SQM_RC, r_mulpolsubs, c_mulpolsubs},
	};

static OpArgInfo ovinfo_db[]=
{
	{T_IGNORED},
	{T_ANS, 0, SIZEOF(ov_ans), ov_ans},
	{T_ROOTS, 0, SIZEOF(ov_roots), ov_roots},
	{T_LDIV, 0, SIZEOF(ov_ldiv), ov_ldiv},
	{T_SAMPLE, 0, SIZEOF(ov_sample), ov_sample},
	{T_INVZ, 0, SIZEOF(ov_invz), ov_invz},
	{T_CROSS, 0, SIZEOF(ov_cross), ov_cross},
	{T_IDEN, 0, SIZEOF(ov_iden), ov_iden},
	{T_REF, 0, SIZEOF(ov_ref), ov_ref},
	{T_RREF, 0, SIZEOF(ov_rref), ov_rref},
	{T_DET, 0, SIZEOF(ov_det), ov_det},
	{T_INV, 0, SIZEOF(ov_inv), ov_inv},
	{T_DIAG, 0, SIZEOF(ov_diag), ov_diag},
	{T_LU, 0, SIZEOF(ov_lu), ov_lu},
	{T_TRACE, 0, SIZEOF(ov_trace), ov_trace},
	{T_DFT, 0, SIZEOF(ov_dft), ov_dft},
	{T_FFT_UNUSED, 0, SIZEOF(ov_dft), ov_dft},
	{T_IDFT, 0, SIZEOF(ov_idft), ov_idft},
	{T_IFFT_UNUSED, 0, SIZEOF(ov_idft), ov_dft},
	{T_CMD, 0, SIZEOF(ov_cmd), ov_cmd},
	{T_FEND},

	{T_TRANSPOSE, 0, SIZEOF(ov_transpose), ov_transpose},
	{T_PLUS, 0, SIZEOF(ov_plus), ov_plus},
	{T_MINUS, 0, SIZEOF(ov_minus), ov_minus},
	{T_ADD_EW, 0, SIZEOF(ov_add_ew), ov_add_ew},
	{T_SUB_EW, 0, SIZEOF(ov_sub_ew), ov_sub_ew},
	{T_MUL, 0, SIZEOF(ov_mul), ov_mul},
	{T_TENSOR, 0, SIZEOF(ov_tensor), ov_tensor},
	{T_DIV, 0, SIZEOF(ov_div), ov_div},
	{T_DIV_BACK, 0, SIZEOF(ov_div_back), ov_div_back},
	{T_MOD, 0, SIZEOF(ov_mod), ov_mod},
	{T_POWER, 0, SIZEOF(ov_power), ov_power},
	{T_ASSIGN, 0, SIZEOF(ov_assign), ov_assign},
	
	{T_ASSIGN_ADD, 1, SIZEOF(ov_assign_add), ov_assign_add},
	{T_ASSIGN_SUB, 1, SIZEOF(ov_assign_sub), ov_assign_sub},
	{T_ASSIGN_MUL, 1, SIZEOF(ov_assign_mul), ov_assign_mul},
	{T_ASSIGN_DIV, 1, SIZEOF(ov_assign_div), ov_assign_div},
	{T_ASSIGN_DIV_BACK, 1, SIZEOF(ov_assign_div_back), ov_assign_div_back},
	{T_ASSIGN_MOD, 1, SIZEOF(ov_assign_mod), ov_assign_mod},
	{T_ASSIGN_POWER, 1, SIZEOF(ov_assign_pow), ov_assign_pow},

	{T_CONTROL_START, 0, SIZEOF(ov_nothing), ov_nothing},//operator nothing
};
const int	opcount=SIZEOF(ovinfo_db);
int			compare_arg(Object const *obj, ArgType argtype)
{
	if(!obj)
		return argtype==A_ABSENT;
	switch(argtype)
	{
	case A_ABSENT:
		return 0;
	case A_SCALAR_REAL_INT:
		return obj->type==T_SCALAR;//don't check here if the scalar real is int
	case A_SCALAR_RC:
		return (obj->type&-2)==T_SCALAR;
	case A_POLY_RC:
		return (obj->type&-2)==T_FRAC&&obj->dy==1&&v_at(obj->r, obj->dx)==1&&(!(obj->type&1)||v_at(obj->i, obj->dx)==0);
	case A_FRAC_RC:
		return (obj->type&-2)==T_FRAC;
	case A_VEC3_RC://cross
		return (obj->type&-2)==T_MATRIX&&(obj->dx==3&&obj->dy==1||obj->dx==1&&obj->dy==3);
	case A_VEC2_RC://cross
		return (obj->type&-2)==T_MATRIX&&(obj->dx==2&&obj->dy==1||obj->dx==1&&obj->dy==2);
	case A_MAT_RC:
		return (obj->type&-2)==T_MATRIX;
	case A_SQM_RC://tr, det, inv, diag, lu
		return (obj->type&-2)==T_MATRIX&&obj->dx==obj->dy;
	case A_POLY_MAT_RC://dft
		return (obj->type&-2)==T_MATRIX
			||(obj->type&-2)==T_FRAC&&obj->dy==1&&v_at(obj->r, obj->dx)==1&&(!(obj->type&1)||v_at(obj->i, obj->dx)==0);
	case A_ANYOBJ:
		return (obj->type&-2)==T_SCALAR||(obj->type&-2)==T_FRAC||(obj->type&-2)==T_MATRIX;
	}
	return 0;
}
void		compile_eval(Expression *ex, int f_idx, int *arg_idx, int nargs, int res_idx, CompileResult *ret)//res_idx is ALWAYS arg_idx[0]
{//f_idx: token index;	arg_idx & res_idx: data idx
	Object *obj1, *obj2;
	static Object A={T_IGNORED}, B={T_IGNORED};
	Token *functok;
	TokenType tt;
	OpArgInfo *ovinfo;
	OpOverload *overload=0;
	int act_args[2], act_nargs, nobj, comp, k;

	//if(!A.r)
	//{
	//	V_CONSTRUCT(double, A.r, 0, 0, 0);
	//	V_CONSTRUCT(double, A.i, 0, 0, 0);
	//	V_CONSTRUCT(double, B.r, 0, 0, 0);
	//	V_CONSTRUCT(double, B.i, 0, 0, 0);
	//}
	functok=&v_at(ex->tokens, f_idx);
	tt=functok->o.type;
	if(tt>=T_SCALAR&&tt<=T_CMATRIX)
		tt=T_CONTROL_START;
	if(tt>=opcount)//
		ASSERT(tt<opcount, "Invalid function token");
	ovinfo=ovinfo_db+tt;
	ASSERT(tt==ovinfo->type, "Corrupt overload info");
	act_nargs=ovinfo->dst_is_arg+nargs;
	if(!act_nargs||act_nargs>2)
	{
		compile_error(functok, "No such overload");
		*ret=C_ERROR;
		return;
	}
	if(ovinfo->dst_is_arg)
		act_args[0]=res_idx, act_args[1]=arg_idx[0];
	else
	{
		act_args[0]=arg_idx[0];
		if(nargs==2)
			act_args[1]=arg_idx[1];
		else
			act_args[1]=-1;
	}
#ifdef DEBUG_COMPILER//check that all obj references are in bounds
	nobj=v_size(&ex->tokens);
	ASSERT(act_args[0]>=0&&act_args[0]<nobj, "nobj = %d, arg1 = %d", nobj, act_args[0]);
	if(act_nargs==2)
		ASSERT(act_args[1]>=0&&act_args[1]<nobj, "nobj = %d, Arg2 = %d", nobj, act_args[1]);
#endif
	obj1=&v_at(ex->tokens, act_args[0]).o;
	if(act_nargs==2)
		obj2=&v_at(ex->tokens, act_args[1]).o;
	else
		obj2=0;
	for(k=0;k<ovinfo->ovcount;++k)
	{
		overload=ovinfo->ov+k;
		if(compare_arg(obj1, overload->a1)&&compare_arg(obj2, overload->a2))
		//if(compare_arg(obj1, overload->a1)&&(act_nargs==1||compare_arg(obj2, overload->a2)))//X  '-1;' CRASH
			break;
	}
	if(k==ovinfo->ovcount)
	{
		compile_error(functok, "No such overload");
		*ret=C_ERROR;
		return;
	}
#ifdef DEBUG_COMPILER//check that all object sizes are consistent
	ASSERT(obj_check(obj1), "Obj %d is corrupt", act_args[0]);
	if(act_nargs==2)
		ASSERT(obj_check(obj2), "Obj %d is corrupt", act_args[1]);
#endif
	comp=!overload->r_func||obj1->type&1||act_nargs==2&&obj2->type&1;
	if(comp)
	{
		if(overload->a1==A_SCALAR_REAL_INT)//int is not promoted
			obj_assign(&A, obj1);
		else
			obj_promotecomplex(&A, obj1);
		if(act_nargs==2)
		{
			if(overload->a2==A_SCALAR_REAL_INT)
				obj_assign(&B, obj2);
			else
				obj_promotecomplex(&B, obj2);
		}
	}
	else
	{
		obj_assign(&A, obj1);
		if(act_nargs==2)
			obj_assign(&B, obj2);
	}
	obj1=&v_at(ex->tokens, res_idx).o;
	if(comp)
	{
		if(!overload->c_func)
		{
			compile_error(functok, "Not supported for complex arguments");
			*ret=C_ERROR;
			return;
		}
		overload->c_func(obj1, &A, &B, functok, ret);
	}
	else
	{
		if(!overload->r_func)
		{
			compile_error(functok, "Not supported for real arguments");
			*ret=C_ERROR;
			return;
		}
		overload->r_func(obj1, &A, &B, functok, ret);
	}
	g_result=res_idx;
	g_modified=1;
}