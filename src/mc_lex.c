//mc_lex.h - MCalc lexer
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
#include	<math.h>
#include	<stdio.h>
#include	<string.h>
#ifdef DEBUG_MEMORY
#include	"mc_redirect_calls.h"
#endif
static const char file[]=__FILE__;
const char	*keywords[]=//TokenType v5
{
		0,
	"ans",
	"roots",
	"ldiv", "sample", "invz",
	"cross",
	"I", "ref", "rref", "det", "inv", "diag", "lu", "tr",//no variables called e, i, j, o
	"dft", "idft", "fft", "ifft",
	"cmd",
		0,
	
	"\'",
	"+", "-", ".+", ".-",
	"*", "o", "/", "\\", "%",
	"^",
	"=", "+=", "-=", "*=", "/=", "\\=", "%=", "^=",
		0,
	"[", "]", "[[", "]]", "(", ")", ",", ";", ";;",
		0,
	"help", "clear", "gfset", "vars", "open",
	"i", "j", "e", "pi", "inf", "nan",
};
const int	nkeys=SIZEOF(keywords);
int			longest_keyword()
{
	int k=0, size=0, maxsize=-1;
	for(k=0;k<nkeys;++k)
	{
		if(keywords[k])
		{
			size=strlen(keywords[k]);
			if(maxsize==-1||maxsize<size)
				maxsize=size;
		}
	}
	return maxsize;
}
int			text_margin_size=0;
const char*	tokentype2str(TokenType a)
{
	int ia=(int)a;
	const char *result=0;

	if(ia<0||a==T_CONTROL_START)
		result="T_IGNORED";
	else if(ia<nkeys)
		result=keywords[ia];
	else
	{
		switch(a)
		{
	//	case T_VAL:			result="T_VAL";			break;
		case T_ID:			result="T_ID";			break;
		case T_SCALAR:		result="T_SCALAR";		break;
		case T_CSCALAR:		result="T_CSCALAR";		break;
		case T_FRAC:		result="T_FRAC";		break;
		case T_CFRAC:		result="T_CFRAC";		break;
		case T_MATRIX:		result="T_MATRIX";		break;
		case T_CMATRIX:		result="T_CMATRIX";		break;
		case T_EQUATION:	result="T_EQUATION";	break;
		}
	}
	sprintf_s(g_buf, g_buf_size, "(%d: %s )", a, result);
	return g_buf;
	//return "???";
}


int			is_whitespace(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
int			is_letter(char c){return c>='A'&&c<='Z'||c>='a'&&c<='z';}
int			is_alphanumeric(char c){return c>='A'&&c<='Z'||c>='a'&&c<='z'||c>='0'&&c<='9';}
int			match_kw(const char *text, const char *kw, int *advance)
{
	int condition=!(strcmp_advance(text, kw, advance)||is_alphanumeric(text[*advance]));
	//if(condition)//
	//	printf("%s\n", kw-1);//
	return condition;
//	return !strcmp_advance(text, kw, advance)&&!is_alphanumeric(text[*advance]);
}
long long	acme_atoll(const char *str, int *advance)
{
	int start, end, neg, k;
	long long p, val;
	if(!str||(str[0]<'0'||str[0]>'9')&&(!str[0]||str[0]!='-'&&str[0]!='+'||str[1]<'0'||str[1]>'9'))
//	if(!str||!(*str>='0'&&*str<='9'||*str&&(*str=='-'||*str=='+')&&str[1]>='0'&&str[1]<='9'))
	{
		if(advance)
			*advance=0;
		return 0;
	}
	start=str[0]=='-'||str[0]=='+', end=start;
	for(;str[end]&&str[end]>='0'&&str[end]<='9';++end);
	p=1, val=0;
	for(k=end-1;k>=start;--k, p*=10)
		val+=(str[k]-'0')*p;
	if(advance)
		*advance=end;
	neg=str[0]=='-';
	return (val^-(long long)neg)+neg;
}
double		acme_atof(const char *str, int *ret_advance)
{
	double val, p;
	int start, k, neg, *pv, e_advance, exponent;
	if(!str||(str[0]<'0'||str[0]>'9')&&(!str[0]||str[0]!='-'&&str[0]!='+'&&str[0]!='.'||(str[1]<'0'||str[1]>'9')&&str[1]!='.'))
//	if(!str||!(*str>='0'&&*str<='9'||*str&&(*str=='-'||*str=='+'||*str=='.')&&(str[1]>='0'&&str[1]<='9'||str[1]=='.')))
	{
		if(ret_advance)
			*ret_advance=0;
		return 0;
	}
	start=str[0]=='.'+((str[0]=='-'||str[0]=='+')+str[1]=='.');
	if(str[start]<'0'||str[start]>'9')
	{
		if(ret_advance)
			*ret_advance=0;
		return 0;
	}
	val=0;
	k=start;
	for(;str[k]&&str[k]>='0'&&str[k]<='9';++k, val*=10)
		val+=str[k]-'0';
	val*=0.1;
	if(str[k]=='.')
	{
		++k;
		for(p=0.1;str[k]&&str[k]>='0'&&str[k]<='9';++k, p*=0.1)
			val+=(str[k]-'0')*p;
	}
	if((str[k]&0xDF)=='E')
	{
		++k;
		e_advance=0;
		exponent=(int)acme_atoll(str+k, &e_advance);
		k+=e_advance;
		val*=_10pow(exponent);
	}
	if(ret_advance)
		*ret_advance=k;
	neg=str[0]=='-', pv=(int*)&val;
	pv[1]|=0x80000000&-neg;
	return val;
}
int			acme_getidentifier(const char *str)
{
	int k=0;
	if(is_letter(*str))
		for(k=1;is_alphanumeric(str[k]);++k);
	return k;
}

//void		ex_lex_push_back_t(LexExpr *le, TokenType a, int b, int line, int col, int len, int pos)
int			lex_push_tok(Expression *ex, TokenType a, unsigned short dx, unsigned short dy, int pos, int line, unsigned short col, unsigned short len)//returns idx of inserted token
{
	int size;
	Token *token;
	size=v_size(&ex->tokens);
	v_resize(&ex->tokens, size+1, 0);//CRASH
	token=ex->tokens+size;
	token->o.type=a;
	token->o.dx=dx;
	token->o.dy=dy;
	token->o.r=0;
	token->o.i=0;
	token->pos=pos;
	token->line=line;
	token->col=col;
	token->len=len;
	return size;

//	Token t={{a, dx, dy}, pos, line, col, len};
//	v_push_back(&ex->tokens, &t);
}
void		lex_push_val_immediate(Expression *ex, int k, int lineno, int linestart, int len, double re, double im)
{
	int idx=lex_push_tok(ex, (TokenType)((int)T_SCALAR+(im!=0)), 1, 1, k, lineno, k-linestart, len);
	Object *obj=&ex->tokens[idx].o;
	V_CONSTRUCT(double, obj->r, 1, &re, 0);
	if(im)
		V_CONSTRUCT(double, obj->i, 1, &im, 0);
}
int			lex_push_val(const char *text, int *k, Expression *ex, int lineno, int linestart)
{
	int advance, idx;
	double val=acme_atof(&v_at(text, *k), &advance);
	Token *token;
	if(advance)//found number
	{
		lex_push_tok(ex, T_SCALAR, 1, 1, *k, lineno, *k-linestart, advance);
		idx=v_size(&ex->tokens)-1;
		token=ex->tokens+idx;

		V_CONSTRUCT(double, token->o.r, 1, &val, 0);

		v_push_back(&ex->idx_data, &idx);

		*k+=advance-1;
		return 1;
	}
	return 0;
}
int			lex_push_id(const char *text, int *k, Expression *ex, int lineno, int linestart)
{
	int advance=acme_getidentifier(text+*k), idx;
	Token *token;
	char *id;
	if(advance)//found identifier
	{
		lex_push_tok(ex, T_ID, 0, 0, *k, lineno, *k-linestart, advance);
		idx=v_size(&ex->tokens)-1;
		token=ex->tokens+idx;

		V_CONSTRUCT(char, token->o.str, advance+1, 0, 0);
		id=token->o.str;
		memcpy(id, text+*k, advance);
		id[advance]='\0';

		v_push_back(&ex->idx_id, &idx);

		*k+=advance-1;
		return 1;
	}
	return 0;
}
int			lex_garbage(const char *text, int *k, Expression *ex, int lineno, int linestart)
{
	Token t={0};
	if(v_at(text, *k)&&!is_whitespace(v_at(text, *k)))
	{
		t.o.type=T_GARBAGE;
		t.pos=*k;
		t.line=lineno;
		t.col=*k-linestart;
		t.len=1;
		compile_error(&t, "Unexpected character");
		++*k;
		return 1;
	}
	return 0;
}
void		lex(const char *text, Expression *ex)//text IS a c vector
{
	int k, size=v_size(&text)-text_margin_size, advance;
	int lineno=0, linestart=0;
	for(k=0;v_at(text, k);++k)
	{
		if(v_at(text, k)=='/')//skip c++ comments, outside of switch for readability
		{
			if(v_at(text, k+1)=='/')
			{
				for(k+=2;k<size&&v_at(text, k)!='\n';++k);
				if(k<size&&v_at(text, k)=='\n')
				{
					++lineno, linestart=k;
					++k;
				}
			}
			else if(v_at(text, k+1)=='*')
			{
				for(k+=2;k<size&&(v_at(text, k)!='*'||v_at(text, k+1)!='/');++k)
					if(v_at(text, k)=='\n')
						++lineno, linestart=k;
				k+=(k<size)<<1;
			}
			if(k>=size)
				break;
		}
		switch(v_at(text, k))
		{
		case ' ':case '\t':case '\r'://skip whitespace
			continue;
		case '\n':
			++lineno, linestart=k;
			break;

			//keywords
		case 'a':
				 if(match_kw(&v_at(text, k+1), keywords[T_ANS		]+1, &advance))	lex_push_tok(ex, T_ANS		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'c':
				 if(match_kw(&v_at(text, k+1), keywords[T_CLEAR		]+1, &advance))	lex_push_tok(ex, T_CLEAR	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_CMD		]+1, &advance))	lex_push_tok(ex, T_CMD		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_CROSS		]+1, &advance))	lex_push_tok(ex, T_CROSS	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'd':
				 if(match_kw(&v_at(text, k+1), keywords[T_DET		]+1, &advance))	lex_push_tok(ex, T_DET		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_DFT		]+1, &advance))	lex_push_tok(ex, T_DFT		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_DIAG		]+1, &advance))	lex_push_tok(ex, T_DIAG		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'e':
				 if(match_kw(&v_at(text, k+1), keywords[T_IMAG_UNUSED]+1, &advance))lex_push_val_immediate(ex, k, lineno, linestart, 1, euler, 0);
			break;
		case 'f':
				 if(match_kw(&v_at(text, k+1), keywords[T_FFT_UNUSED]+1, &advance))	lex_push_tok(ex, T_DFT		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'g':
				 if(match_kw(&v_at(text, k+1), keywords[T_GFSET		]+1, &advance))	lex_push_tok(ex, T_GFSET	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'h':
				 if(match_kw(&v_at(text, k+1), keywords[T_HELP		]+1, &advance))	lex_push_tok(ex, T_HELP		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'I':
				 if(match_kw(&v_at(text, k+1), keywords[T_IDEN		]+1, &advance))	lex_push_tok(ex, T_IDEN		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'i':
				 if(match_kw(&v_at(text, k+1), keywords[T_IDFT		]+1, &advance))	lex_push_tok(ex, T_IDFT		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_IFFT_UNUSED]+1, &advance))lex_push_tok(ex, T_IDFT		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_INF		]+1, &advance))	lex_push_val_immediate(ex, k, lineno, linestart, 1, _HUGE, 0), k+=2;
			else if(match_kw(&v_at(text, k+1), keywords[T_INVZ		]+1, &advance))	lex_push_tok(ex, T_INVZ		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_INV		]+1, &advance))	lex_push_tok(ex, T_INV		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_IMAG		]+1, &advance))	lex_push_val_immediate(ex, k, lineno, linestart, 1, 0, 1);
			break;
		case 'j':
				 if(match_kw(&v_at(text, k+1), keywords[T_IMAG_UNUSED]+1, &advance))lex_push_val_immediate(ex, k, lineno, linestart, 1, 0, 1);
			break;
		case 'l':
				 if(match_kw(&v_at(text, k+1), keywords[T_LDIV		]+1, &advance))	lex_push_tok(ex, T_LDIV		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_LU		]+1, &advance))	lex_push_tok(ex, T_LU		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'o':
				 if(match_kw(&v_at(text, k+1), keywords[T_OPEN		]+1, &advance))	lex_push_tok(ex, T_OPEN		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_TENSOR	]+1, &advance))	lex_push_tok(ex, T_TENSOR	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'p':
				 if(match_kw(&v_at(text, k+1), keywords[T_PI		]+1, &advance))	lex_push_val_immediate(ex, k, lineno, linestart, 1, pi, 0);
			break;
		case 'r':
				 if(match_kw(&v_at(text, k+1), keywords[T_REF		]+1, &advance))	lex_push_tok(ex, T_REF		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_ROOTS		]+1, &advance))	lex_push_tok(ex, T_ROOTS	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			else if(match_kw(&v_at(text, k+1), keywords[T_RREF		]+1, &advance))	lex_push_tok(ex, T_RREF		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 's':
				 if(match_kw(&v_at(text, k+1), keywords[T_SAMPLE	]+1, &advance))	lex_push_tok(ex, T_SAMPLE	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 't':
				 if(match_kw(&v_at(text, k+1), keywords[T_TRACE		]+1, &advance))	lex_push_tok(ex, T_TRACE	, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;
		case 'v':
				 if(match_kw(&v_at(text, k+1), keywords[T_VARS		]+1, &advance))	lex_push_tok(ex, T_VARS		, 0, 0, k, lineno, k-linestart, advance+1), k+=advance;
			break;

			//symbols & operators
		case '+':// += +
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_ADD		, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_PLUS				, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '-':// -= -
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_SUB		, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_MINUS			, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '.':// .+ .- numbers
				 if(v_at(text, k+1)=='+')	lex_push_tok(ex, T_ADD_EW			, 0, 0, k, lineno, k-linestart, 2), ++k;
			else if(v_at(text, k+1)=='-')	lex_push_tok(ex, T_SUB_EW			, 0, 0, k, lineno, k-linestart, 1);
			else if(!lex_push_val(text, &k, ex, lineno, linestart))
				lex_garbage(text, &k, ex, lineno, linestart);
			break;
		case '*':// *= *
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_MUL		, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_MUL				, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '/':// /= /
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_DIV		, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_DIV				, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '\\':// \= \	stop
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_DIV_BACK	, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_DIV_BACK			, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '%':// %
			lex_push_tok(ex, T_MOD, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '^':// ^= ^
				 if(v_at(text, k+1)=='=')	lex_push_tok(ex, T_ASSIGN_POWER		, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_POWER			, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '=':
			lex_push_tok(ex, T_ASSIGN, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '\'':// '
			lex_push_tok(ex, T_TRANSPOSE, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '[':// [[ [
				 if(v_at(text, k+1)=='[')	lex_push_tok(ex, T_MATSTART			, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_POLSTART			, 0, 0, k, lineno, k-linestart, 1);
			break;
		case ']':// ]] ]
				 if(v_at(text, k+1)==']')	lex_push_tok(ex, T_MATEND			, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_POLEND			, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '(':
			lex_push_tok(ex, T_LPR, 0, 0, k, lineno, k-linestart, 1);
			break;
		case ')':
			lex_push_tok(ex, T_RPR, 0, 0, k, lineno, k-linestart, 1);
			break;
		case ',':
			lex_push_tok(ex, T_COMMA, 0, 0, k, lineno, k-linestart, 1);
			break;
		case ';':// ;; ;
				 if(v_at(text, k+1)==';')	lex_push_tok(ex, T_QUIET			, 0, 0, k, lineno, k-linestart, 2), ++k;
			else							lex_push_tok(ex, T_SEMICOLON		, 0, 0, k, lineno, k-linestart, 1);
			break;
		case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
			lex_push_val(text, &k, ex, lineno, linestart);
			break;
		default:
			if(!lex_push_id(text, &k, ex, lineno, linestart))
				lex_garbage(text, &k, ex, lineno, linestart);
			break;
		}//end switch
	}//end for
}
void		lexer_init()
{
	text_margin_size=longest_keyword()+1;
}