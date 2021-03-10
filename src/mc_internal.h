//mc_internal.h - MCalc internal include
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

#ifndef MC_INTERNAL_H
#define MC_INTERNAL_H
#include	"mc.h"
extern int	result;

//objects
int			obj_check(Object const *obj);
const char*	tokentype2str(TokenType a);
void		ex_print_ub(Expression const *ex, int start, int end);

void		obj_assign(Object *dst, Object const *src);
void		obj_assign_immediate(Object *dst, TokenType type, unsigned short dx, unsigned short dy, double *re, double *im);
void		obj_assign_cvec(Object *dst, TokenType type, unsigned short dx, unsigned short dy, double **pre, double **pim);//uses v_move
void		obj_promotecomplex(Object *dst, Object const *src);
void		obj_setscalar(Object *obj, double re, double im);

//interpreter internals
extern const char *exstr;//different from the one in mc_main.c
void		compile_eval(Expression *ex, int f_idx, int *arg_idx, int nargs, int res_idx, CompileResult *ret);
void		compile_error(Token *token, const char *msg, ...);
#endif