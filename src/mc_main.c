#include"mc.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>//memcpy
#include<stdarg.h>//va_list
#include<math.h>
#include<tmmintrin.h>//SSSE3
static const char file[]=__FILE__;
#ifdef DEBUG_MEMORY
#include"mc_redirect_calls.h"
#endif

extern const int	g_buf_size=G_BUF_SIZE;
char				g_buf[G_BUF_SIZE]={0};
//void		print_rand_bits()
//{
//	int k, r;
//	for(k=0;k<10;++k)
//	{
//		r=rand()<<30|rand()<<15|rand();
//		mem_print_bin(&r, 4);
//		printf("\n");
//	}
//	printf("\n");
//}

static char	*exstr=0;//input string
Expression		expr={0};
CompileError	*compileerrors=0;
Object			*ans=0;
GlobalObject	*vars=0;

char		gfmode=0;//later
void		print_help()
{
	printf(
		"MCALC: A Matrix Calculator\n"
		"Usage:\n"
		"  mcalc [\"expression\"/filename]\n"
		"\n"
		"Syntax:\n"
		"  [a2, a1, a0]             is for polynomials: (a2 x^2 + a1 x + a0)\n"
		"  [[a1, a2, a3]]           is a row vector\n"
		"  [[a11, a12;  a21, a22]]  is a 2x2 matrix\n"
		"\n"
		"  A B  is multiplication, except:\n"
		"       when A is a polynomial or a fraction object and B is a square matrix\n"
		"       it is substitution of B into A: A(B)\n"
		"\n"
		"  Expressions end with semicolon ;\n"
		"  Elements MUST be separated by commas ,\n"//rewrite: more graphical, less reading
		"  Matrix rows are separated by semicolons ;\n"
		"  Expressions ending with double semicolon ;; don't produce output\n"
		"  Nested matrices and/or polynomials are not supported\n"
		"\n"
		"Operators by precedence (first to last):\n"
		"  ^\n"
		"  + - (unary)\n"
		"  \'\n"
		"  / \\ %% * o - .- + .+\n"
		"  = += -= *= /= \\= %%=\n"
		"  ,\n"
		"  ( )\n"
		"  [ ] ; [[ ]]\n"
		"  ;;\n"
		"Notes:\n"
		"  \'     is transpose (eg: M\')\n"
		"  o     is the tensor product\n"
		"  \\     is for square matrices (eg: A\\B)\n"
	//	"  %%     makes fraction objects (eg: F=num%%den)\n"
		"  .+ .- are element-wise operations\n"
		"\n"
		"Keywords:\n"
		"  help: Print this info (works in cmd)\n"
		"  clear: Clears all variables from memory\n"
		"  vars: Shows all variables & answer count\n"
	//	"  vars: Shows all variables in memory\n"
	//	"  open: Choose a text file to open\n"
		"General functions:\n"
		"  ans(n): The n-th latest answer\n"
		"Vectors:\n"
		"  cross(3D vec, 3D vec) -> 3D vec\n"
		"  cross(2D vec, 2D vec) -> scalar\n"
		"Matrices:\n"
	//	"  join: \n"//X flatten NESTED MATRICES
		"  ref: Row Echelon Form\n"
		"  rref: Reduced Row Echelon Form (Gaussian Elimination)\n"
		"Square matrices:\n"
		"  I n: n-square identity matrix\n"//autosize?
		"  det: determinant of square matrix\n"
		"  inv: inverse of square matrix\n"
		"  diag: diagonal factorization of square matrix\n"
		"  lu: LU factorization of square matrix\n"
		"  tr: trace of square matrix\n"
	//	"Polynomials:\n"
	//	"  roots: find the roots\n"
		"Fraction objects:\n"
		"  sample(F): s to z domain\n"
		"  ldiv(F, S): long division of fraction F by S steps\n"
	//	"  plot(F, S): long division\n"
	//	"Matrices & polynomials:\n"
	//	"  dft/fft: Discrete Fourier Transform\n"
	//	"  idft/ifft: Inverse Discrete Fourier Transform\n"
		"\n"
		);
}
int			proc_end(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
	_getch();
	return 0;
}
void		get_exstr_interactive(void *pexstr, const char *cmdstr)
{
	char *cret;
	if(gfmode)
		printf("gf");
	if(cmdstr)
		printf(cmdstr);
	printf("> ");
	cret=fgets(g_buf, g_buf_size-1, stdin);
	if(!cret)
	{
		printf("Failed to read input\n");
	//	proc_end("Failed to read input\n");
		return;
	}
	v_insert(pexstr, v_size(pexstr)-1, g_buf, strlen(g_buf)-1);//insert before null terminator, excluding newline in g_buf
}

int			workonce=0;
int			main(int argc, const char **argv)//project started on 2021-01-21
{
	CompileResult comres=C_ERROR;
	const char strempty[]="", ellipsis[]="..";
	char c;
	int exstr_size;
	int anscount0=0, anscount, k;

	//wchar_t wbuf2[1024]={0};
	//swprintf_s(wbuf2, 1024, L"ASCII: %S\nWCHAR: %s\n", "LOL_1", L"LOL_2");
	//swprintf_s(wbuf2, 1024, L"ASCII: %s\nWCHAR: %ls\n", "LOL_1", L"LOL_2");

	printf("MCALC%s\n\n", argc==2?"":"\t\tCtrl C to exit.");

	print_help();//
	
	lexer_init();
	V_CONSTRUCT(char, exstr, 1, strempty, 0);
	ex_construct(&expr);
	
	V_CONSTRUCT(CompileError, compileerrors, 0, 0, 0);
	V_CONSTRUCT(Object, ans, 0, 0, 0);
	V_CONSTRUCT(GlobalObject, vars, 0, 0, 0);
	
	if(argc>2)
	{
		printf(
			"Usage:\n"
			"  mcalc [\"expression\"/filename]\n"
			"Please enclose command arguments in doublequotes.\n"
		//	"Please enclose expressions or filename in doublequotes \" when passing command arguments.\n"
			"Press \'h\'for help, \'x\' to exit, or any key to continue.\n");
		c=_getch();
		if((c&0xDF)=='H')
			print_help();
		else if((c&0xDF)=='X')
			return 0;
		get_exstr_interactive(&exstr, 0);
	}
	else if(argc==2)
	{
		v_insert(&exstr, 0, argv[1], strlen(argv[1]));
		printf("> %s\n\n", exstr);
	}
	else
		get_exstr_interactive(&exstr, 0);

	for(;;)
	{
		for(;;)
		{
			exstr_size=v_size(&exstr);
			v_resize(&exstr, exstr_size+text_margin_size, strempty);
			lex(exstr, &expr);
#ifdef PRINT_COMPILE_INTERNALS
			ex_print(&expr);//
#endif
			comres=compile(exstr, &expr);
			compile_print_errors(compileerrors);
			if(comres==C_OK)
				break;
			if(comres==C_ERROR)
				v_clear(&exstr);
			else
			{
				exstr_size=v_size(&exstr);
				v_resize(&exstr, exstr_size-text_margin_size, 0);
			}
			ex_clear(&expr);

			get_exstr_interactive(&exstr, ellipsis);
		}
		if(v_size(&ans)>0)
		{
			anscount=v_size(&ans);
			for(k=anscount0;k<anscount;++k)
			{
				printf("ans(%d) =\n", anscount-1-k);//answers are counted upwards: ans0 is the latest
				obj_print_reusable(ans+k);
			}
			anscount0=anscount;
		}
		else//
			obj_print_reusable(&expr.tokens[expr.result].o);//
		
		//cleanup
		v_erase(&exstr, 0, v_size(&exstr)-1);//leave null terminator
		exstr_size=v_size(&exstr);//
		ex_clear(&expr);

		get_exstr_interactive(&exstr, 0);
		exstr_size=v_size(&exstr);//
	}
	//_getch();
	return 0;
}