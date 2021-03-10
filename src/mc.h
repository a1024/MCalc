//mc.h - MCalc main include file
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

#ifndef MC_H
#define MC_H
#ifdef _WIN32
#include<conio.h>
#else
char	_getch();
char	_getche();
#endif

//	#define		USE_SIMD

	#define		ENABLE_ASSERT//don't comment this till release
	#define		CVECTOR_CHECKED//use v_at() instead of [] for optional bound checking unless pointer is ofsetted, don't increment idx inside v_at()
	#define		DEBUG_COMPILER//checks for reference & object consistency
//	#define		PRINT_COMPILE_INTERNALS	//normally disabled
//	#define		DEBUG_CVECTOR			//normally disabled
//	#define     DEBUG_MEMORY			//normally disabled


#define	SIZEOF(STATIC_ARRAY)	sizeof(STATIC_ARRAY)/sizeof(*(STATIC_ARRAY))

#define		G_BUF_SIZE	65536
extern const int	g_buf_size;
extern char			g_buf[G_BUF_SIZE];

//system
void get_console_size(short *w, short *h);

//math
extern const double pi, euler;
int			maximum(int a, int b);
int			minimum(int a, int b);
int			mod(int x, int n);
int			floor_log2(unsigned long long n);
int			floor_log10(double x);
double		power(double x, int y);
double		_10pow(int n);

//complex
void		comp_inv(double *dst_re, double *dst_im, double Ar, double Ai);
void		comp_mul(double *dst_re, double *dst_im, double Ar, double Ai, double Br, double Bi);
void		comp_div(double *dst_re, double *dst_im, double Ar, double Ai, double Br, double Bi);


//error handling
enum DebugEventType
{
	DE_INFO,
	DE_PROGRESS,
	DE_URGENT,
};
extern int debugmode;
void debugevent(int level, const char *format, ...);
const char* get_filename(const char *filename);
int crash(const char *file, int line, const char *expr, const char *msg, ...);
#define LOG_INFO(MSG, ...)	debugevent(DE_INFO, __VA_ARGS__)
#define LOG_PROG(MSG, ...)	debugevent(DE_PROGRESS, __VA_ARGS__)
#define	LOG_ERROR(MSG, ...)	debugevent(DE_URGENT, __VA_ARGS__)
#ifdef ENABLE_ASSERT
#define ASSERT(SUCCESS, MSG, ...)	(void)((SUCCESS)!=0||crash(file, __LINE__, #SUCCESS, MSG, __VA_ARGS__))
#else
#define	ASSERT(...)
#endif


#ifdef DEBUG_MEMORY
void*   d_alloc		(const char *file, int line, unsigned long bytesize);
void*   d_realloc	(const char *file, int line, void *p, unsigned long bytesize);
void    d_free		(const char *file, int line, void *p);
void	d_memcpy	(const char *file, int line, void *dst, const void *src, int bytesize);
#endif


int		strcmp_advance(const char *s1, const char *s2, int *advance);
void	mem_test(const char *file, int line, const char *msg, ...);
#define	MEMTEST(MSG, ...)	mem_test(file, __LINE__, MSG, __VA_ARGS__)
void	mem_shiftback(void *dst, const void *src, int bytesize);
void	mem_shiftforward(const void *src, void *dst, int bytesize);
void	mem_rotate(void *begin, void *newbegin, const void *end);
void	memfill(void *dst, const void *src, int dstbytes, int srcbytes);//repeating pattern
//#if 0
//void	mem_fillpattern2(const char *file, int line, void *p, int bytesize, int esize, const void *data);
//#define mem_fillpattern(P, BYTESIZE, ESIZE, DATA)	mem_fillpattern2(file, __LINE__, P, BYTESIZE, ESIZE, DATA)
//#else
//void	mem_fillpattern(void *p, int bytesize, int esize, const void *data);
//#endif
void	mem_print(const void *buf, int bytesize);
void	mem_print_bin(const void *buf, int bytesize);

//IDEA: c vector v3: recursive destructor, manage reallocs

//c vector v4
typedef struct
{
	int count;
//	int capacity;
	short esize, ploc_size;
#ifdef DEBUG_CVECTOR
	int type_size, name_size;
#endif
	union
	{
		int *payload;
		char *cpayload;
		long long unused;//header should contain an 8 byte variable for padding, to contain doubles without DEBUG_CVECTOR (long double is useless?)
	};
} CVecHeader;
#ifdef DEBUG_CVECTOR
#define	CVEC_DEBUG_ARGS	, const char *type, const char *name
#define	V_CONSTRUCT(ETYPE, NAME, COUNT, P_ELEM, PTRLOC)		v_construct(&NAME, COUNT, sizeof(ETYPE), P_ELEM, PTRLOC, #ETYPE, #NAME)
#else
#define	CVEC_DEBUG_ARGS
#define	V_CONSTRUCT(ETYPE, NAME, COUNT, P_ELEM, PTRLOC)		v_construct(&NAME, COUNT, sizeof(ETYPE), P_ELEM, PTRLOC)
#endif

//base functions
void	v_construct(void *pv, int count, int esize, const void *elem, const int *pointerlocations	CVEC_DEBUG_ARGS);
void	v_move(void *pdst, void *psrc);
void	v_destroy(void *pv);
void	v_insert(void *pv, int idx, const void *begin, int count);
void	v_erase(void *pv, int idx, int count);
void	v_check(const char *file, int line, const void *pv, int idx);
#ifdef CVECTOR_CHECKED//don't increment idx inside v_at()
#define	v_at(VEC, IDX)	(*(v_check(file, __LINE__, &(VEC), IDX), (VEC)+(IDX)))
#else
#define	v_at(VEC, IDX)	(VEC)[IDX]
#endif

//derived functions
void	v_assign(void *pdst, const void *psrc);
int		v_size(const void *pv);
void	v_push_back(void *pv, const void *data);
void	v_pop_back(void *pv);
void	v_append(void *pv, const void *begin, int count);
void	v_resize(void *pv, int count, const void *data);
void	v_fill(void *pv, const void *elem);
void	v_clear(void *pv);

//helpers
void	v_setheader(void *pv, int count, int esize, int *payload);
void	v_getheader(const void *pv, int *pcount, int *pesize, int **payload);
CVecHeader*	v_getptr(void *pv);
void	v_print(const void *pv, const char *msg, ...);


//mcalc
typedef enum//TokenType v5
{
		T_IGNORED,//ignored by compiler, negative token numbers are ignored too

	//functions
	T_ANS,//general
	T_ROOTS,//polynomials
	T_LDIV, T_SAMPLE, T_INVZ,//fractions
	T_CROSS,//row vectors
	T_IDEN, T_REF, T_RREF, T_DET, T_INV, T_DIAG, T_LU, T_TRACE,//matrices
	T_DFT, T_FFT_UNUSED, T_IDFT, T_IFFT_UNUSED,//matrices & polynomials

		T_FEND,
	
	//operators
	T_TRANSPOSE,
	T_PLUS, T_MINUS, T_ADD_EW, T_SUB_EW,//+ - can be unary
	T_MUL, T_TENSOR, T_DIV, T_DIV_BACK, T_MOD,
	T_POWER,

	//assign
	T_ASSIGN,
	T_ASSIGN_ADD, T_ASSIGN_SUB,
	T_ASSIGN_MUL, T_ASSIGN_DIV, T_ASSIGN_DIV_BACK, T_ASSIGN_MOD,
	T_ASSIGN_POWER,
	
		T_CONTROL_START,//used as operator nothing, see mc_eval.c
		
	//control
	T_POLSTART, T_POLEND,
	T_MATSTART, T_MATEND,
	T_LPR, T_RPR,
	T_COMMA, T_SEMICOLON, T_QUIET,

		T_CONTROL_END,

	//commands
	T_HELP, T_CLEAR, T_GFSET, T_VARS, T_OPEN,

	//constant keywords
	T_IMAG, T_IMAG_UNUSED, T_EULER, T_PI, T_INF, T_NAN,

	//additional tokens
	T_ID,//identifier: token->o.sdata

	//objects (must be even):		token->o.r/i
	T_SCALAR,	T_CSCALAR,	//dx==dy==1 always
	T_FRAC,		T_CFRAC,	//contiguous d={num[dx], den[dy]},	polynomial: dy=0
	T_MATRIX,	T_CMATRIX,	//dy * dx
	T_EQUATION,	//?

	T_GARBAGE,//lex error
} TokenType;
//typedef struct
//{
//	double r, i;
//} Complex;

//interpreter 2
typedef struct//32:16 TTTT AA BB RRRR IIII	64:24 TTTT AA BB RRRRRRRR IIIIIIII
{
	TokenType type;
	unsigned short dx, dy;//object dimensions < 65536
	//union
	//{
	//	Complex d[];
	//	char str[];
	//};
	union
	{
		struct{double *r, *i;};
		char *str;
	};
} Object;
typedef struct//PPPP LLLL CC EE  32: 16+12=28  64: 24+12=36
{
	Object o;
	int pos, line;//<2GB file
	unsigned short col, len;//linelen<65536, tokenlen<65536
} Token;
typedef struct
{
	Token *tokens;
	int *idx_data,//data object positions in token array: tokens[data_idx[k]].o.r/i
		*idx_id;//identifier positions in token array: tokens[id_idx[k]].o.str
	int result;//token index
} Expression;
typedef struct//32:16+4=20	64:24+8=32
{
	Object o;
	char *name;
} GlobalObject;

typedef enum
{
	CE_INFO,
	CE_WARNING,
	CE_ERROR,
} CompileErrorType;
typedef struct
{
	short type, col, len;
	int pos, line;
	char *msg;
} CompileError;
typedef enum
{
	C_OK,
	C_OK_BUT_INCOMPLETE,
	C_ERROR
} CompileResult;
extern CompileError	*compileerrors;
extern Object		*ans;
extern GlobalObject	*vars;


//objects
void		ex_construct(Expression *ex);
void		ex_clear(Expression *ex);
void		ex_print(Expression const *ex);
void		obj_print_reusable(Object const *obj);


//lexer
extern int	text_margin_size;//resize string by this ammount before lexing
void		lexer_init();
void		lex(const char *text, Expression *ex);//text IS a c vector
void		ex_clear(Expression *ex);


//compiler
//CompileResult	compile(const char *text, Expression *ex, CompileError *errors, Object *ans, GlobalObject *vars);
CompileResult	compile(const char *text, Expression *ex);
int				compile_print_errors(CompileError const *errors);
#endif//MC_H