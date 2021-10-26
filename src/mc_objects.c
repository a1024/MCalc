//mc_objects.h - MCalc objects
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

#include	"mc_internal.h"
#include	<stdio.h>
#include	<string.h>
#include	<math.h>
static const char file[]=__FILE__;
int			obj_check(Object const *obj)//true on success
{
	int rsize=v_size(&obj->r), isize;
	switch(obj->type)
	{
	case T_SCALAR:
		return rsize==1&&obj->dx==1&&obj->dy==1;
	case T_FRAC:
		return rsize==obj->dx+obj->dy;
	case T_MATRIX:
		return rsize==obj->dx*obj->dy;
	}
	isize=v_size(&obj->i);
	switch(obj->type)
	{
	case T_CSCALAR:
		return rsize==1&&isize==1&&obj->dx==1&&obj->dy==1;
	case T_CFRAC:
		return rsize==obj->dx+obj->dy&&isize==obj->dx+obj->dy;
	case T_CMATRIX:
		return rsize==obj->dx*obj->dy&&isize==obj->dx*obj->dy;
	}
	return 0;//fail
}
void		obj_assign(Object *dst, Object const *src)
{
	if(dst!=src)
	{
		if(src->type<0)
		{
			printf("obj_assign(): src->type < 0 (assigning an invalid object)\n");//
			return;
		}
		dst->type=src->type;
		dst->dx=src->dx;
		dst->dy=src->dy;
		if(src->type==T_ID)
			v_assign(&dst->str, &src->str);
		else
		{
			//if(!src->r)//
			//	printf("obj_assign(): src->r == 0\n");//
			v_assign(&dst->r, &src->r);
			if(src->i)
				v_assign(&dst->i, &src->i);
			else if(dst->i)
				v_destroy(&dst->i);
		}
	}
}
void		obj_promotecomplex(Object *dst, Object const *src)
{
	int count;
	obj_assign(dst, src);
	if(!(src->type&1))
	{
		++dst->type;
	//	ASSERT(!dst->i, "Corrupt object");
		count=v_size(&dst->r);
		if(dst->i)
			v_resize(&dst->i, count, 0);
		else
			V_CONSTRUCT(double, dst->i, count, 0, 0);
		memset(dst->i, 0, count*sizeof(double));
	}
}
static int	deduce_count(TokenType type, unsigned short dx, unsigned short dy)
{
	int count;
	switch(type&-2)
	{
	case T_SCALAR:	count=1;		break;
	case T_FRAC:	count=dx+dy;	break;
	case T_MATRIX:	count=dx*dy;	break;
	default:		count=0;		break;
	}
	ASSERT(count, "Invalid assignment");
//	ASSERT(count, "Attempting to assign unknown object");
	return count;
}
void		obj_assign_immediate(Object *dst, TokenType type, unsigned short dx, unsigned short dy, double *re, double *im)//NOT c vectors
{
	int count=deduce_count(type, dx, dy);
	if(dst->type==T_ID)
	{
		v_destroy(&dst->str);
		V_CONSTRUCT(double, dst->r, count, 0, 0);
	}
	else
		v_resize(&dst->r, count, 0);
	memcpy(dst->r, re, count*sizeof(double));
	if(type&1)
	{
		if(dst->type&1)
			v_resize(&dst->i, count, 0);
		else
			V_CONSTRUCT(double, dst->i, count, 0, 0);
		memcpy(dst->i, im, count*sizeof(double));
	}
	else if(dst->type&1)
		v_destroy(&dst->i);
	dst->type=type;
	dst->dx=dx;
	dst->dy=dy;
}
void		obj_assign_cvec(Object *dst, TokenType type, unsigned short dx, unsigned short dy, double **pre, double **pim)//pre & pim ARE c vectors
{
	int count=deduce_count(type, dx, dy);
	if(dst->type==T_ID)
		v_destroy(&dst->str);
	v_move(&dst->r, pre);
	if(type&1)
		v_move(&dst->i, pim);
	else if(dst->type&1)
		v_destroy(&dst->i);
	dst->type=type;
	dst->dx=dx;
	dst->dy=dy;
}
void		obj_setscalar(Object *obj, double re, double im)
{
	v_resize(&obj->r, 1, 0);
	obj->r[0]=re;
	if(im)
	{
		if(obj->i)
		{
			v_resize(&obj->i, 1, 0);
			obj->i[0]=im;
		}
		else
			V_CONSTRUCT(double, obj->i, 1, &im, 0);
	}
	else if(obj->i)
		v_destroy(&obj->i);
	obj->dx=obj->dy=1;
	obj->type=(TokenType)(T_SCALAR+(im!=0));
}

void		ex_construct(Expression *ex)
{
	V_CONSTRUCT(Token, ex->tokens, 0, 0, 0);
	V_CONSTRUCT(int, ex->idx_data, 0, 0, 0);
	V_CONSTRUCT(int, ex->idx_id, 0, 0, 0);
}
//void		ex_form_meta(Token const *expr, int **pdata_idx, int **pid_idx)
//{
//	int size=v_size(&expr), k;
//	Object const *obj;
//	if(!*pdata_idx)
//		V_CONSTRUCT(int, *pdata_idx, 0, 0, 0);
//	else
//		v_clear(pdata_idx);
//	if(!*pid_idx)
//		V_CONSTRUCT(int, *pid_idx, 0, 0, 0);
//	else
//		v_clear(pid_idx);
//	for(k=0;k<size;++k)
//	{
//		obj=&expr[k].o;
//		if(obj->type==T_ID)
//			v_push_back(pid_idx, &k);
//		else if(obj->type>=T_SCALAR&&obj->type<=T_CMATRIX)
//			v_push_back(pdata_idx, &k);
//	}
//}
void		ex_clear(Expression *ex)
{
	Object *obj;
	int size=v_size(&ex->tokens), ndata=v_size(&ex->idx_data), nid=v_size(&ex->idx_id), k, toktype;

	ASSERT(ex->tokens, "Expression \'tokens\' pointer is NULL.");
	ASSERT(ex->idx_data, "Expression \'idx_data\' pointer is NULL.");
	ASSERT(ex->idx_id, "Expression \'idx_id\' pointer is NULL.");
	for(k=0;k<ndata;++k)//free objects
	{
		obj=&ex->tokens[ex->idx_data[k]].o;
		toktype=abs(obj->type);
		ASSERT(toktype>=T_SCALAR&&toktype<=T_CMATRIX, "Invalid metadata: idx_data[%d]=%d points at a non-object.", k, ex->idx_data[k]);
		v_destroy(&obj->r);
		if(toktype&1)
		{
			ASSERT(obj->i, "Invalid metadata: Expected imaginary component, but the pointer is NULL.");
			v_destroy(&obj->i);
		}
	}
	for(k=0;k<nid;++k)//free identifiers
	{
		obj=&ex->tokens[ex->idx_id[k]].o;
		toktype=abs(obj->type);
		ASSERT(toktype==T_ID, "Invalid metadata: idx_id[%d]=%d points at a non-identifier.", k, ex->idx_id[k]);
		v_destroy(&obj->str);
	}
	//for(k=0;k<size;++k)
	//{
	//	obj=&ex->tokens[k].o;
	//	if(obj->type==T_ID)
	//		v_destroy(&obj->sdata);
	//	else if(obj->type>=T_SCALAR&&obj->type<=T_CMATRIX)
	//	{
	//		v_destroy(&obj->r);
	//		if(obj->type&1)
	//			v_destroy(&obj->i);
	//	}
	//}
	v_clear(&ex->tokens);
	v_clear(&ex->idx_data);
	v_clear(&ex->idx_id);
}

int			print_scalarcomplex(double re, double im)
{
	int printed=0;
	if(re)
		printed+=printf("%g", re);
	if(im)
	{
		if(printed)
		{
			if(im>0)
				printed+=printf(" + ");
			else
				printed+=printf(" - "), im=-im;
		}
		printed+=printf("%g i", im);
	}
	if(!printed)
		printed+=printf("0");
	return printed;
}
void		print_poly_reusable(double *r, double *i, int ncoeff)
{
	int k;
	printf("[");
	if(i)
	{
		for(k=0;k<ncoeff;++k)
		{
			print_scalarcomplex(r[k], i[k]);
			if(k+1<ncoeff)
				printf(", ");
		}
	}
	else
	{
		for(k=0;k<ncoeff;++k)
		{
			printf("%g", r[k]);
			if(k+1<ncoeff)
				printf(", ");
		}
	}
	printf("]");
}
void		print_frac_reusable(double *r, double *i, int nnum, int nden)
{
	print_poly_reusable(r, i, nnum);//print num
	if(!(nden==1&&r[nnum]==1&&(!i||i[nnum]==0)))
	{
		printf("\n/\n");
		print_poly_reusable(r+nnum, i?i+nnum:0, nden);//print den
	}
	printf(";\n");
}
void		print_matrix_reusable(double *r, double *i, int dx, int dy)
{
	int ky, kx, idx;
	printf("[[\n");
	if(i)
	{
		for(ky=0, idx=0;ky<dy;++ky)
		{
			printf("\t");
			for(kx=0;kx<dx;++kx, ++idx)
			{
				print_scalarcomplex(r[idx], i[idx]);
				if(kx+1<dx)
					printf(", ");
			}
			if(ky+1<dy)
				printf(";\n");
		}
	}
	else
	{
		for(ky=0, idx=0;ky<dy;++ky)
		{
			printf("\t");
			for(kx=0;kx<dx;++kx, ++idx)
			{
				printf("%g", r[idx]);
				if(kx+1<dx)
					printf(", ");
			}
			if(ky+1<dy)
				printf(";\n");
		}
	}
	printf("\n]];\n");
}
void		obj_print_reusable(Object const *obj)
{
	switch(obj->type)
	{
	case T_SCALAR:
		printf("%g;\n", obj->r[0]);
		break;
	case T_CSCALAR:
		print_scalarcomplex(obj->r[0], obj->i[0]);
		printf(";\n");
		break;
	case T_FRAC:
	case T_CFRAC:
		print_frac_reusable(obj->r, obj->i, obj->dx, obj->dy);
		break;
	case T_MATRIX:
	case T_CMATRIX:
		print_matrix_reusable(obj->r, obj->i, obj->dx, obj->dy);
		break;
	}
}
void		ex_print(Expression const *ex)
{
	int ntokens, nobj, nid;
	int k, k2;
	Object const *obj;

	printf("\nExpression:\n");
	ntokens=v_size(&ex->tokens);
	nobj=v_size(&ex->idx_data);
	nid=v_size(&ex->idx_id);
	for(k=0;k<ntokens;++k)
	{
		obj=&ex->tokens[k].o;
	//	if(obj->type>=T_ID&&obj->type<=T_CMATRIX)
			printf("[%d]%s  ", k, tokentype2str(obj->type));
	}
	printf("\n\nNumbers:\n");
	for(k=0;k<nobj;++k)
	{
		k2=ex->idx_data[k];
		obj=&ex->tokens[k2].o;
		printf("[%d]%s:\n", k2, tokentype2str(obj->type));
		obj_print_reusable(obj);
	}
	printf("\nIdentifiers:\n");
	for(k=0;k<nid;++k)
	{
		k2=ex->idx_id[k];
		obj=&ex->tokens[k2].o;
		printf("[%d]%s: %s\n", k2, tokentype2str(obj->type), obj->str);
	}
}
void		ex_print_ub(Expression const *ex, int start, int end)//unary-binary info
{
	int k;
	Token const *t;
	Object const *obj;
	printf("\n");
	for(k=start;k<end;++k)
	{
		t=ex->tokens+k;
		obj=&t->o;
		if(obj->type>=T_SCALAR&&obj->type<=T_CMATRIX)
			printf("[%d]%s  ", k, tokentype2str(obj->type));
		else if(obj->type>T_IGNORED&&obj->type<T_CONTROL_START)
			printf("%s[%c]  ", tokentype2str(obj->type), (char)obj->dx);
		else
			printf("%s  ", tokentype2str(obj->type));
	}
	printf("\n\n");
}