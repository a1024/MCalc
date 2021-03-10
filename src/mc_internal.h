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