//mc_parse.h - MCalc parser
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

//interpreter
#include	"mc_internal.h"
#include	<stdio.h>
#include	<string.h>//memset
static const char file[]=__FILE__;
int			enable_warnings=0;
const char	*exstr=0;

//int			is_objstart(TokenType t)
//{
//	static const TokenType objtypes[]={T_IGNORED, T_SCALAR, T_FRAC, T_MATRIX, T_EQUATION};
//	return objtypes[(t==T_SCALAR)|(t==T_POLSTART)<<1|(t==T_MATSTART)*3];
//}
int			is_control(TokenType t){return t>T_CONTROL_START||t<T_CONTROL_END;}
void		compile_error(Token *token, const char *msg, ...)
{
	CompileError error={token->line, token->col, token->len, token->pos};
	int msglen=vsnprintf_s(g_buf, g_buf_size, g_buf_size-1, msg, (char*)(&msg+1));
	V_CONSTRUCT(char*, error.msg, 0, 0, 0);
	v_append(&error.msg, g_buf, msglen+1);
	v_push_back(&compileerrors, &error);
}
void		compile_error_clear()
{
	int k, size=v_size(&compileerrors);
	for(k=0;k<size;++k)
		v_destroy(&v_at(compileerrors, k).msg);
	v_clear(&compileerrors);
}
int			compile_print_errors(CompileError const *errors)
{
	CompileError const *error;
	int k, size=v_size(&compileerrors);
	if(size)
	{
		if(v_at(compileerrors, size-1).line)
		{
			for(k=0;k<size;++k)
			{
				error=compileerrors+k;
				printf("line %d col %d \'%.*s\': %s\n", error->line+1, error->col+1, error->len, exstr+error->pos, error->msg);
			//	printf("line %d col %d %s\n", error->line, error->col, error->msg);
			//	printf("(%d:%d) \'%.*s\' %s\n", error->line, error->col, error->msg);
			}
		}
		else
		{
			for(k=0;k<size;++k)
			{
				error=compileerrors+k;
				printf("at %d \'%.*s\': %s\n", error->col+1, error->len, exstr+error->pos, error->msg);
			}
		}
		printf("Type \'help\' to show syntax\n");
	//	printf("\'help\' shows syntax\n");
		return 1;
	}
	return 0;
}

/*void		compile_print(LexExpr const *le, Expression const *ex)
{
	int k2;
	Token const *t;
	Object const *obj;
	printf("\nExpr:\n");
	for(k2=0;k2<v_size(&le->tokens);++k2)
	{
		t=le->tokens+k2;
		if(t->a>=T_VAL&&t->a<T_GARBAGE)
			printf("%s[%d]  ", tokentype2str(t->a), t->b);
		else
			printf("%s  ", tokentype2str(t->a), t->b);
	}
	printf("\nObjects:\n");
	for(k2=0;k2<v_size(&ex->data);++k2)
	{
		obj=ex->data+k2;
		switch(obj->type)
		{
		case T_SCALAR:
			printf("[%d] T_SCALAR: %g\n", k2, *obj->d);
			break;
		case T_FRAC:
			printf("[%d] T_FRAC:\n", k2);
			print_fraction(obj);
			break;
		case T_MATRIX:
			printf("[%d] T_MATRIX:\n", k2);
			print_matrix(obj);
			break;
		}
	}
	//	printf("[%d]\t%g\t\n", k2, v_at(le->fdata, k2));
	printf("\nStrings:\n");
	for(k2=0;k2<v_size(&le->sdata);++k2)
		printf("[%d]\t%.*s\t\n", k2, v_size(&v_at(le->sdata, k2)), v_at(le->sdata, k2));
	printf("\n");
}//*/
//void		ex_clear(Expression *ex)
//{
//	int k, size=v_size(&ex->data);
////	v_clear(&ex->in);
//	for(k=0;k<size;++k)
//	{
//		v_destroy(&v_at(ex->data, k).d);
//		v_destroy(&v_at(ex->data, k).name);
//	}
//	v_clear(&ex->data);
//}

typedef enum
{
	PREC_UNKNOWN,

	PREC_POWER,
	PREC_SIGN,
	PREC_TRANSPOSE,
	PREC_FUNC,
	PREC_DIV, PREC_MUL,
	PREC_SUB, PREC_ADD,
	PREC_ASSIGN,

	PREC_LAST,
} OpPrec;
typedef struct
{
	char//TODO: swap postfix & prefix variables
		post, post_middle,
		bin, rtl,//rtl: only needed when bin is true
		pre, pre_middle;
	char	bin_prec, u_prec;
} OperatorInfo;
const OperatorInfo op_info[]=//access: op_info[token->a-T_FEND]		note different RTL with same prec is ambiguous
{//	 >	>m	b	RTL	<	<m	bp				up
	{0,	0,	0,	0,	1,	1,	0,				PREC_FUNC		},//T_FEND (all unary functions)
	{1,	1,	0,	0,	0,	0,	0,				PREC_TRANSPOSE	},//T_TRANSPOSE
	{0,	0,	1,	0,	1,	1,	PREC_ADD,		PREC_SIGN		},//T_PLUS
	{0,	0,	1,	0,	1,	1,	PREC_SUB,		PREC_SIGN		},//T_MINUS
	{0,	0,	1,	0,	0,	0,	PREC_ADD,		0				},//T_ADD_EW
	{0,	0,	1,	0,	0,	0,	PREC_SUB,		0				},//T_SUB_EW
	{0,	0,	1,	0,	0,	0,	PREC_MUL,		0				},//T_MUL
	{0,	0,	1,	0,	0,	0,	PREC_MUL,		0				},//T_TENSOR
	{0,	0,	1,	0,	1,	0,	PREC_DIV,		PREC_SIGN		},//T_DIV
	{1,	0,	1,	1,	0,	0,	PREC_DIV,		PREC_SIGN		},//T_DIV_BACK		ambiguous
	{0,	0,	1,	0,	0,	0,	PREC_DIV,		PREC_TRANSPOSE	},//T_MOD		NO PERCENT - ONLY MOD
	{0,	0,	1,	1,	0,	0,	PREC_POWER,		0				},//T_POWER
	{0,	0,	1,	1,	0,	0,	PREC_ASSIGN,	0				},//T_ASSIGN* (all assignments)
	{0,	0,	1,	1,	0,	0,	PREC_MUL,		0,				},//T_ASSIGN+1 (operator nothing)
	{0,	0,	0,	0,	0,	0,	0,				0,				},//T_ASSIGN+2 (unknown)
};
OperatorInfo const* get_opinfo(TokenType op)
{
	if(op>=T_SCALAR&&op<=T_CMATRIX)
		op=T_ASSIGN+1;//term
	else if(op>T_IGNORED&&op<T_FEND)
		op=T_FEND;//all unary functions
	else if(op>=T_ASSIGN&&op<T_CONTROL_START)
		op=T_ASSIGN;//all assigns
	else if(op==T_IGNORED||op>=T_CONTROL_START)
		op=T_ASSIGN+2;//uknown
	return op_info+op-T_FEND;
}
void		compile_flat(Expression *ex, int start, int end, CompileResult *ret)
{
	int kn0, kn, lk, rk, k;
	Token *token;
	Object *obj;
	char dir='_';
	char found_bin_pre;
	OperatorInfo const *oinf;
	char lprec, rprec;
	char go_left, go_right;
	char lbin, rbin;
	int args[2]={-1, -1};
	TokenType *tt;
	
	g_modified=0;
	//check operators & set type: dx = 'b', '<' or '>'
	for(lk=start;lk<end;++lk)//everything before any anchors is on left
	{
		obj=&ex->tokens[lk].o;
		if(obj->type>=T_SCALAR&&obj->type<T_EQUATION)
			break;
		if(obj->type>0)
			obj->dx='<';
	}
	for(;lk<end;)//bin select loop
	{
		for(rk=lk+1;rk<end;++rk)//find anchor on right
		{
			token=ex->tokens+rk;
			obj=&token->o;
			if(obj->type>=T_SCALAR&&obj->type<T_EQUATION)
				break;
		}
		if(rk==end)
			break;
		//one binary (else error), others are postfix or prefix unary
		//first encountered bin or pre_middle -capable is taken as such
		found_bin_pre=0;
		for(k=lk+1;k<rk;++k)
		{
			token=ex->tokens+k;
			obj=&token->o;
			if(obj->type>0)
			{
				oinf=get_opinfo(obj->type);
				if(found_bin_pre)//upost or bin
				{
					if(oinf->pre_middle)
						obj->dx='<', found_bin_pre|=2;
					else
					{
						compile_error(token, "Not allowed after a %s operator", found_bin_pre&1?"binary":"prefix-unary");
						*ret=C_ERROR;
						tt=&obj->type, *tt=-*tt;
					}
				}
				else
				{
					if(oinf->bin)
						obj->dx='b', found_bin_pre|=1;
					else if(oinf->pre_middle)
						obj->dx='<', found_bin_pre|=2;
					else if(oinf->post_middle&&!found_bin_pre)
						obj->dx='>';
				}
			}
		}
		lk=rk;
	}//end bin select loop
	for(++lk;lk<end;++lk)//everything after all anchors is on right
	{
		obj=&ex->tokens[lk].o;
		if(obj->type>0)
			obj->dx='>';
	}

	//parse expression
	kn0=-1, kn=start;
	for(;;++kn)
	{
#ifdef PRINT_COMPILE_INTERNALS
		ex_print_ub(ex, start, end);
#endif
		for(;kn<end;++kn)//find next anchor
		{
			token=ex->tokens+kn;
			obj=&token->o;
			if(obj->type>=T_ID&&obj->type<=T_CMATRIX)
				break;
		}
		if(kn==end)//reached the end
		{
			for(--kn;kn>=start;--kn)//find last anchor
			{
				token=ex->tokens+kn;
				obj=&token->o;
				if(obj->type>=T_ID&&obj->type<=T_CMATRIX)
					break;
			}
		}
compile_flat_search:
		for(lk=kn-1;lk>=start;--lk)//find left operator/anchor
			if(ex->tokens[lk].o.type>0)
				break;
		for(rk=kn+1;rk<end;++rk)//find right operator/anchor
			if(ex->tokens[rk].o.type>0)
				break;
		if(lk>=start)
		{
			if(rk<end)//both left & right found: select which one to take
			{
				token=ex->tokens+lk;
				obj=&token->o;
				oinf=get_opinfo(obj->type);
				go_left=!oinf->rtl;
				if(obj->dx=='b')
					lprec=oinf->bin_prec, lbin=1;
				else if(obj->dx=='<')
					lprec=oinf->u_prec, lbin=0;
				else
					lprec=PREC_LAST, lbin=0;
				
				token=ex->tokens+rk;
				obj=&token->o;
				oinf=get_opinfo(obj->type);
				go_right=oinf->rtl;
				if(obj->dx=='b')
					rprec=oinf->bin_prec, rbin=1;
				else if(obj->dx=='>')
					rprec=oinf->u_prec, rbin=0;
				else
					rprec=PREC_LAST, rbin=0;

				if(lprec<rprec)
					dir='<';
				else if(lprec>rprec)
					dir='>';
				else if(go_left>go_right)
					dir='<';
				else if(go_left<go_right)
					dir='>';
				else
				{
					compile_error(token, "Ambiguous with \'%.*s\'", token->len, exstr+token->pos);
					*ret=C_ERROR;
					tt=&ex->tokens[lk].o.type, *tt=-*tt;
					tt=&ex->tokens[rk].o.type, *tt=-*tt;
					goto compile_flat_search;
				}
			}
			else
				dir='<';
		}
		else
		{
			if(rk<end)
				dir='>';
			else//no operators found
				break;
		}
		if(dir=='<')
		{
			obj=&ex->tokens[lk].o;
			if((obj->type>=T_SCALAR&&obj->type<=T_CMATRIX)||obj->dx=='b')//binary (including operator nothing)
			{
				//if(kn0==-1)
				//{
				//	printf("PARSE ERROR: [%d] %.*s [%d]\n\n", kn0, ex->tokens[lk].len, exstr+ex->tokens[lk].pos, kn);
				//	ex_print(ex);
				//	_getch();
				//	return;
				//}
				for(kn0=kn-1;kn0>=start;--kn0)
				{
					token=ex->tokens+kn0;
					obj=&token->o;
					if(obj->type>=T_ID&&obj->type<=T_CMATRIX)
						break;
				}
				if(kn0<start)
				{
					printf("PARSE ERROR:\nrange=[%d, %d[\n[%d] %.*s [%d]\n\n", start, end, kn0, ex->tokens[lk].len, exstr+ex->tokens[lk].pos, kn);
					ex_print(ex);
					_getch();
					return;
				}
				args[0]=kn0, args[1]=kn;
				compile_eval(ex, lk, args, 2, args[0], ret);
				tt=&ex->tokens[lk].o.type;
				if(*tt<T_SCALAR||*tt>T_CMATRIX)//not operator nothing
					*tt=-*tt;
				tt=&ex->tokens[kn].o.type, *tt=-*tt;
			}
			else//prefix-unary
			{
				args[0]=kn;
				compile_eval(ex, lk, args, 1, args[0], ret);
				tt=&ex->tokens[lk].o.type, *tt=-*tt;
			}
		}
		else if(dir=='>')
		{
			obj=&ex->tokens[rk].o;
			if((obj->type<T_SCALAR||obj->type>T_CMATRIX)&&obj->dx=='>')//if unary-postfix (operator nothing is always binary)
			{
				args[0]=kn;
				compile_eval(ex, rk, args, 1, args[0], ret);
				tt=&ex->tokens[rk].o.type, *tt=-*tt;
			}//if right binary: continue
		}
	//	kn0=kn;
	}//end expr parse loop
}
void		compile_expr(Expression *ex, int start, int end, CompileResult *ret)
{
	int k, lstart=start, lend=end,
		parlevel=0, parlevel_max=0, parlevel_peak=0, callevel;
	TokenType toktype, *tt;
	int *argloc=0, nargs, argstart;
	g_modified=0;
	V_CONSTRUCT(int, argloc, 0, 0, 0);
	for(;;)//level loop
	{
		lstart=start, lend=end;
		parlevel=0, parlevel_max=0, parlevel_peak=0;
		for(k=start;k<end;++k)//find highest parenthesis level
		{
			toktype=v_at(ex->tokens, k).o.type;
			if(toktype==T_LPR)
			{
				++parlevel;
				if(parlevel_max<parlevel)
					parlevel_max=parlevel, lstart=k+1, parlevel_peak=1;
			}
			else if(toktype==T_RPR)
			{
				--parlevel;
				if(parlevel<0)//extra closing parens
				{
					if(lstart==start)
						lend=k;
					break;
				}
				if(parlevel_peak)
					parlevel_peak=0, lend=k;
			}
		}
		if(lstart>start)
			tt=&v_at(ex->tokens, lstart-1).o.type, *tt=-*tt;
		if(lend<end)
			tt=&v_at(ex->tokens, lend).o.type, *tt=-*tt;
		if(parlevel_max>0&&lstart-2>=0)
		{
			toktype=v_at(ex->tokens, lstart-2).o.type;
			callevel=toktype>T_IGNORED&&toktype<T_FEND;
		}
		else
			callevel=0;
		if(callevel)
		{
			for(argstart=k=lstart;;++k)
			{
				tt=&v_at(ex->tokens, k).o.type;
				if(k==lend||*tt==T_COMMA)
				{
					compile_flat(ex, argstart, k, ret);
					if(k<lend)
						*tt=-*tt;//clear comma
					if(g_modified)
						v_push_back(&argloc, &g_result);
					else
						v_push_back(&argloc, &argstart);
					if(k==lend)
						break;
					argstart=k+1;
				}
			}
			nargs=v_size(&argloc);
			if(nargs)
			{
				compile_eval(ex, lstart-2, argloc, nargs, v_at(argloc, 0), ret);
				tt=&v_at(ex->tokens, lstart-2).o.type, *tt=-*tt;
				for(argstart=1;argstart<nargs;++argstart)//clear all call args, except argloc[0]
				{
					tt=&v_at(ex->tokens, argloc[argstart]).o.type;
					ASSERT(*tt>=0, "Failed to clear call args\n");
					*tt=-*tt;
				}
			}
			else//unreachable
			{
				compile_error(&v_at(ex->tokens, lstart-1), "Missing argument list");
				*ret=C_ERROR;
			}
		}
		else
			compile_flat(ex, lstart, lend, ret);
		if(lstart==start&&lend==end)
			break;
	}//end level loop
	v_destroy(&argloc);
}
//CompileResult compile(const char *text, Expression *ex, CompileError *errors, Object *ans, GlobalObject *vars)
CompileResult compile(const char *text, Expression *ex)
{
	static const TokenType objtypes[]={T_IGNORED, T_FRAC, T_MATRIX, T_EQUATION};
	int ntokens=v_size(&ex->tokens), k, objstart, start;
	Object obj={T_SCALAR};
	int pollevel=0, matlevel=0,
		parlevel_out=0, parlevel_in=0,//out: between objects, in: inside an object
		ncommas_first=0, ncommas=0, nsemicolons=0;
	CompileResult ret=C_OK;
	TokenType token, token2, *tt;
	double re, im;
	int assignment;
	Object *pobj;
	GlobalObject *pgobj;
	char *varname;
	int varname_len;

	exstr=text;

	//syntax check
#if 1
	for(k=0;k<ntokens;++k)//check assignments, bracket integrity and object dimensions			no nested brackets, brackets & semicolon override parens, parens override commas
	{
		token=v_at(ex->tokens, k).o.type;
		if(token>=T_ASSIGN&&token<T_CONTROL_START)
		{
			if(pollevel|matlevel|(parlevel_out>0))
			{
				compile_error(ex->tokens+k, "Assignment can only be at global scope");
				ret=C_ERROR;
				goto compile_returnpoint1;
			}
			if(k)
				token2=v_at(ex->tokens, k-1).o.type;
			if(!k||token2!=T_ID)
			{
				compile_error(ex->tokens+k, "Missing assigned variable");
				ret=C_ERROR;
				goto compile_returnpoint1;
			}
			if(k-1>0)
			{
				token2=v_at(ex->tokens, k-2).o.type;
				if(token2!=T_COMMA||token2!=T_SEMICOLON)//constexpr on left of assignment
				{
				//	compile_error(ex->tokens+k, "Expected an lvalue");
					compile_error(ex->tokens+k, "Expected a variable name");
				//	compile_error(ex->tokens+k, "Expression on left of assignment");
				//	compile_error(ex->tokens+k, "Expected an identifier");
					ret=C_ERROR;
					goto compile_returnpoint1;
				}
			}
		}//end if assignment
		switch(token)
		{
		case T_POLSTART:
			++pollevel;
			break;
		case T_POLEND:
			if(enable_warnings&&parlevel_in)
				compile_error(ex->tokens+k, "Warning: Parenthesis mismatch");
			--pollevel, parlevel_in=0;
			ncommas=0;
			break;
		case T_MATSTART:
			++matlevel;
			break;
		case T_MATEND:
			if(enable_warnings&&parlevel_in)
				compile_error(ex->tokens+k, "Warning: Parenthesis mismatch");
			--matlevel, parlevel_in=0;
			if(ncommas_first&&ncommas_first!=ncommas)// >1 row: last row must match
			{
				compile_error(ex->tokens+k, "Row mismatch");
				ret=C_ERROR;
				goto compile_returnpoint1;
			}
			ncommas_first=0;
			ncommas=0;
			nsemicolons=0;
			break;
		case T_LPR:
			if(pollevel|matlevel)
				++parlevel_in;
			else
				++parlevel_out;
			break;
		case T_RPR:
			if(pollevel|matlevel)
				--parlevel_in;
			else
				--parlevel_out;
			break;
		case T_COMMA:
			if(pollevel|matlevel&&parlevel_in<=0||parlevel_out<=0)
				++ncommas;
			break;
		case T_SEMICOLON:
			if(pollevel|matlevel)
				++nsemicolons;
			if(pollevel)
			{
				compile_error(ex->tokens+k, "Not expected in a polynomial");
				ret=C_ERROR;
				goto compile_returnpoint1;
			}
			if(matlevel)
			{
				if(!ncommas_first)//first row
					ncommas_first=ncommas;
				else if(ncommas_first!=ncommas)//following rows must match
				{
					compile_error(ex->tokens+k, "Row mismatch");
					ret=C_ERROR;
					goto compile_returnpoint1;
				}
			}
			ncommas=0;
			break;
		case T_QUIET:
			if(pollevel|matlevel)
			{
				compile_error(ex->tokens+k, "Can only be at the end of a statement");
				ret=C_ERROR;
				goto compile_returnpoint1;
			}
			break;
		}//end switch
		if(pollevel<0||matlevel<0)//closed without opening
		{
			compile_error(ex->tokens+k, "Bracket mismatch");
			ret=C_ERROR;
			goto compile_returnpoint1;
		}
		if(pollevel+matlevel>1)//opened twice, or opened both
		{
			compile_error(ex->tokens+k, "Nested matrices and/or polynomials are not supported");
			ret=C_ERROR;
			goto compile_returnpoint1;
		}
	}
	if(pollevel||matlevel)//don't compile until string is complete
	{
		ret=C_OK_BUT_INCOMPLETE;
		goto compile_returnpoint1;
	}
	if(ntokens)//code must be terminated by ; or ;;
	{
		token=v_at(ex->tokens, ntokens-1).o.type;
		if(token!=T_SEMICOLON&&token!=T_QUIET)
		{
			ret=C_OK_BUT_INCOMPLETE;
			goto compile_returnpoint1;
		}
	}
#endif
	
	for(k=0;k<ntokens;)//parse polynomials & matrices
	{
		for(;k<ntokens;++k)//find a polynomial or a matrix
		{
			pobj=&v_at(ex->tokens, k).o;
			obj.type=objtypes[(pobj->type==T_POLSTART)|(pobj->type==T_MATSTART)<<1];
			if(obj.type>0)
				break;
		}
		if(k>=ntokens)//not found
			break;
		
		if(obj.type==T_FRAC||obj.type==T_MATRIX)//found poly or matrix
		{
			objstart=k;
			V_CONSTRUCT(double, obj.r, 0, 0, 0);
			obj.i=0;
			obj.dx=obj.dy=1;

			parlevel_in=0;
			for(++k;k<ntokens;++k)
			{
				start=k;
				for(;k<ntokens;++k)//skip to end of element
				{
					token=v_at(ex->tokens, k).o.type;
					parlevel_in+=(token==T_LPR)-(token==T_RPR);
					if(parlevel_in<=0&&token==T_COMMA||token==T_POLEND||token==T_SEMICOLON||token==T_MATEND)
						break;
				}
				obj.dx+=(obj.dy==1)&(token==T_COMMA);
				obj.dy+=token==T_SEMICOLON;
				if(start<k)
				{
					compile_expr(ex, start, k, &ret);
					pobj=&v_at(ex->tokens, g_modified?g_result:start).o;
					tt=&pobj->type, *tt=-*tt;//clear elem token
					if(!pobj->r)
						ASSERT(pobj->r, "Result index points at a non-object");//
					re=v_at(pobj->r, 0);
					if(pobj->type==T_CSCALAR)
						im=v_at(pobj->i, 0);
					else
						im=0;
				}
				else
					re=im=0;
				v_push_back(&obj.r, &re);//add to the object

				if(obj.type&1&&obj.i)
					v_push_back(&obj.i, &im);
				else if(im)//upgrade to complex
				{
					++obj.type;
					V_CONSTRUCT(double, obj.i, v_size(&obj.r), 0, 0);
					v_at(obj.i, v_size(&obj.r)-1)=im;
				}
				//if(im)
				//{
				//	if(obj.type&1&&obj.i)
				//		v_push_back(&obj.i, &im);
				//	else
				//	{
				//		++obj.type;
				//		V_CONSTRUCT(double, obj.i, 1, &im, 0);
				//	}
				//}

				tt=&v_at(ex->tokens, k).o.type, *tt=-*tt;//clear control token
				if(token==T_POLEND||token==T_MATEND)
					break;
			}
			if(obj.type==T_FRAC||obj.type==T_CFRAC)//append denominator
			{
				re=1;
				v_push_back(&obj.r, &re);
				if(obj.type==T_CFRAC)
				{
					im=0;
					v_push_back(&obj.i, &im);
				}
			}
			++k;//skip T_POLEND or T_MATEND
			if(k<ntokens)
			{
				token=v_at(ex->tokens, k).o.type;
				k+=(token==T_COMMA)|(token==T_SEMICOLON);
			}
			if(obj.dx==1&&obj.dy==1)
			{
				if((obj.type&-2)==T_FRAC)
				{
					v_resize(&obj.r, 1, 0);
					if(obj.type&1)
						v_resize(&obj.i, 1, 0);
				}
				obj.type=T_SCALAR+(obj.type&1);
			}
			//if(obj.type==T_MATRIX&&obj.dx==1&&obj.dy==1||obj.type==T_FRAC&&obj.dx==1)//dy==1 always for poly
			//	obj.type=T_SCALAR;
			//if(obj.type==T_CMATRIX&&obj.dx==1&&obj.dy==1||obj.type==T_CFRAC&&obj.dx==1)
			//	obj.type=T_CSCALAR;

			pobj=&v_at(ex->tokens, objstart).o;
			obj_assign(pobj, &obj);
			v_push_back(&ex->idx_data, &objstart);//small memory leak
			g_result=objstart;
			//v_at(ex->tokens, objstart).o.type=obj.type;
			//v_at(le->tokens, objstart).b=v_size(&ex->data);
			//v_push_back(&ex->data, &obj);//push object to ex
		}//end poly or matrix
	}//end parse objects
#ifdef PRINT_COMPILE_INTERNALS
	ex_print(ex);
#endif

	parlevel_out=0;
	for(start=0, k=0;;++k)//compile object [assignment] expressions
	{
		if(k<ntokens)
		{
			token=v_at(ex->tokens, k).o.type;
			parlevel_out+=(token==T_LPR)-(token==T_RPR);
		}
		if(k>=ntokens||(token==T_COMMA&&parlevel_out<=0||token==T_SEMICOLON||token==T_QUIET)&&start<k)
		{
			token=v_at(ex->tokens, start+1).o.type;
			assignment=v_at(ex->tokens, start).o.type==T_ID&&token>=T_ASSIGN&&token<T_CONTROL_START;
			if(start+(assignment<<1)<k)
				compile_expr(ex, start+(assignment<<1), k, &ret);//compile expression
			else//empty assignment
			{
				compile_error(ex->tokens+k, "Empty assignment statement");
				ret=C_ERROR;
				continue;
			//	goto compile_returnpoint1;
			}
			
			pobj=&v_at(ex->tokens, g_result).o;
			//pobj=&v_at(ex->tokens, g_modified?g_result:start+(assignment<<1)).o;
			//pobj=&v_at(ex->tokens, start+(assignment<<1)).o;
			//switch(pobj->type&-2)
			//{
			//case T_SCALAR:count=1;break;
			//case T_FRAC:	count=pobj->dx+pobj->dy;break;
			//case T_MATRIX:count=pobj->dx*pobj->dy;break;
			//default:		count=0;break;
			//}
			//obj.type=pobj->type;
			//V_CONSTRUCT(double, obj.r, 0, 0, 0);
			//v_append(&obj.r, v_at(ex->tokens, start+(assignment<<1)).o.r, count);
			if(assignment)//append result to global object array
			{
				varname=v_at(ex->tokens, start).o.str;
				varname_len=strlen(varname);

				v_push_back(vars, 0);
				pgobj=vars+v_size(&vars)-1;

				V_CONSTRUCT(char, pgobj->name, varname_len+1, 0, 0);
				memcpy(pgobj->name, varname, varname_len);
				pgobj->name[varname_len]='\0';

				obj_assign(&pgobj->o, pobj);
			}
			else//append result to ans array
			{
				v_push_back(&ans, 0);
				obj_assign(ans+v_size(&ans)-1, pobj);
			}
			k+=k<ntokens;//skip control token
			start=k;
		}//end if
		if(k>=ntokens)
			break;
	}//end ground compile loop

compile_returnpoint1:
	return ret;
}