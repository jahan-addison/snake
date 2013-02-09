%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taz.h"
#include "smach.h"
#include "gram.h"

extern struct mempool symbolnamepool, macrotextpool;

static char *fixup_string(char *);
static char *fixup_macarg(char *);
int falseclause=0;
%}

ws	[ \t]
digit	[0-9]
decimal	[1-9][0-9]*
octal	0[0-7]*
hex	0[xX][0-9a-fA-f]+
hex2	$[0-9a-fA-f]+
binary  "%"[0-1]+
number	{decimal}|{octal}|{hex}

trailcomment ;
linecomment [#*]
label [a-zA-Z_.][a-zA-Z$_.0-9]*

%s OPCODE OPERAND
%x COMMENT FALSECLAUSE MACRO MACRO2 MACARG

%%


\/\*([^*]|\*+[^*/])\*+\/	{ char *p=yytext; while(*p) if(*p++=='\n') current_lineno++; numlines++; }
<COMMENT>[^\n]*			;
{trailcomment}			BEGIN(COMMENT);
<INITIAL>{ws}*{linecomment}	BEGIN(COMMENT);

<INITIAL>{ws}+			BEGIN(falseclause? FALSECLAUSE:OPCODE);
<INITIAL,OPCODE>{label}:	if(falseclause) BEGIN(FALSECLAUSE); else { BEGIN(OPCODE); if(yytext[0]!='.') flushlocalsyms(); yytext[strlen(yytext)-1]='\0'; yylval.str=poolstring(&symbolnamepool, yytext); return T_LABEL; }
<INITIAL>{label}		if(falseclause) BEGIN(FALSECLAUSE); else { BEGIN(OPCODE); if(yytext[0]!='.') flushlocalsyms(); yylval.str=poolstring(&symbolnamepool, yytext); return T_LABEL; }

<MACARG>\,			return T_COMMA;
<MACARG>{ws}+			;
<MACARG>[^ \t\n,;][^\n,;]*	{ /* FIX  Hardcoded ; */ yylval.str=fixup_macarg(yytext); return T_MACARG; }
<MACARG>{trailcomment}[^\n]*    { BEGIN(INITIAL); return T_EMA; }


<OPCODE>[Aa][Dd][Dd]	{ BEGIN(OPERAND); return T__ADD; }
<OPCODE>[Aa][Dd][Dd][Cc]	{ BEGIN(OPERAND); return T_ADDC; }
<OPCODE>[Aa][Nn][Dd]	{ BEGIN(OPERAND); return T__AND; }
<OPCODE>[Bb][Ee]	{ BEGIN(OPERAND); return T_BE; }
<OPCODE>[Bb][Nn]	{ BEGIN(OPERAND); return T_BN; }
<OPCODE>[Bb][Nn][Ee]	{ BEGIN(OPERAND); return T_BNE; }
<OPCODE>[Bb][Nn][Zz]	{ BEGIN(OPERAND); return T_BNZ; }
<OPCODE>[Bb][Pp]	{ BEGIN(OPERAND); return T_BP; }
<OPCODE>[Bb][Pp][Cc]	{ BEGIN(OPERAND); return T_BPC; }
<OPCODE>[Bb][Rr]	{ BEGIN(OPERAND); return T_BR; }
<OPCODE>[Bb][Rr][Ff]	{ BEGIN(OPERAND); return T_BRF; }
<OPCODE>[Bb][Zz]	{ BEGIN(OPERAND); return T_BZ; }
<OPCODE>[Cc][Aa][Ll][Ll]	{ BEGIN(OPERAND); return T_CALL; }
<OPCODE>[Cc][Aa][Ll][Ll][Ff]	{ BEGIN(OPERAND); return T_CALLF; }
<OPCODE>[Cc][Aa][Ll][Ll][Rr]	{ BEGIN(OPERAND); return T_CALLR; }
<OPCODE>[Cc][Ll][Rr][1Q]	{ BEGIN(OPERAND); return T_CLR1; }
<OPCODE>[Dd][Bb][Nn][Zz]	{ BEGIN(OPERAND); return T_DBNZ; }
<OPCODE>[Dd][Ee][Cc]	{ BEGIN(OPERAND); return T_DEC; }
<OPCODE>[Dd][Ii][Vv]	{ BEGIN(OPERAND); return T__DIV; }
<OPCODE>[Ii][Nn][Cc]	{ BEGIN(OPERAND); return T_INC; }
<OPCODE>[Jj][Mm][Pp]	{ BEGIN(OPERAND); return T_JMP; }
<OPCODE>[Jj][Mm][Pp][Ff]	{ BEGIN(OPERAND); return T_JMPF; }
<OPCODE>[Ll][Dd]	{ BEGIN(OPERAND); return T_LD; }
<OPCODE>[Ll][Dd][Cc]	{ BEGIN(OPERAND); return T_LDC; }
<OPCODE>[Mm][Uu][Ll]	{ BEGIN(OPERAND); return T__MUL; }
<OPCODE>[Mm][Oo][Vv]	{ BEGIN(OPERAND); return T_MOV; }
<OPCODE>[Nn][Oo][Pp]	{ BEGIN(OPERAND); return T_NOP; }
<OPCODE>[Nn][Oo][Tt][1Q]	{ BEGIN(OPERAND); return T_NOT1; }
<OPCODE>[Oo][Rr]	{ BEGIN(OPERAND); return T__OR; }
<OPCODE>[Pp][Oo][Pp]	{ BEGIN(OPERAND); return T_POP; }
<OPCODE>[Pp][Uu][Ss][Hh]	{ BEGIN(OPERAND); return T_PUSH; }
<OPCODE>[Rr][Ee][Tt]	{ BEGIN(OPERAND); return T_RET; }
<OPCODE>[Rr][Ee][Tt][Ii]	{ BEGIN(OPERAND); return T_RETI; }
<OPCODE>[Rr][Oo][Ll]	{ BEGIN(OPERAND); return T_ROL; }
<OPCODE>[Rr][Oo][Ll][Cc]	{ BEGIN(OPERAND); return T_ROLC; }
<OPCODE>[Rr][Oo][Rr]	{ BEGIN(OPERAND); return T_ROR; }
<OPCODE>[Rr][Oo][Rr][Cc]	{ BEGIN(OPERAND); return T_RORC; }
<OPCODE>[Ss][Ee][Tt][1Q]	{ BEGIN(OPERAND); return T_SET1; }
<OPCODE>[Ss][Tt]	{ BEGIN(OPERAND); return T_ST; }
<OPCODE>[Ss][Uu][Bb]	{ BEGIN(OPERAND); return T__SUB; }
<OPCODE>[Ss][Uu][Bb][Cc]	{ BEGIN(OPERAND); return T_SUBC; }
<OPCODE>[Xx][Cc][Hh]	{ BEGIN(OPERAND); return T_XCH; }
<OPCODE>[Xx][Oo][Rr]	{ BEGIN(OPERAND); return T__XOR; }


<OPCODE>\.?[sS][eE][cC][tT][iI][oO][nN]	{ BEGIN(OPERAND); return T_SECTION; }
<OPCODE>\.?[eE][nN][dD]			{ BEGIN(OPERAND); return T_END; }
<OPCODE>\.?[eE][qQ][uU]			{ BEGIN(OPERAND); return T_EQU; }
<OPCODE>\.?[sS][eE][tT]			{ BEGIN(OPERAND); return T_SET; }
<OPCODE>\.?[iI][nN][cC][lL][uU][dD][eE]	{ BEGIN(OPERAND); return T_INCLUDE; }
<OPCODE>\.?[iI][nN][cC][dD][iI][rR]	{ BEGIN(OPERAND); return T_INCDIR; }
<OPCODE>\.?[eE][nN][dD][mM]		{ BEGIN(OPERAND); return T_ENDM; }
<OPCODE>\.?[mM][eE][xX][iI][tT]		{ BEGIN(OPERAND); return T_MEXIT; }
<OPCODE>\.?[bB][yY][tT][eE]		{ BEGIN(OPERAND); return T_BYTE; }
<OPCODE>\.?[wW][oO][rR][dD]		{ BEGIN(OPERAND); return T_WORD; }
<OPCODE>\.?[oO][rR][gG]			{ BEGIN(OPERAND); return T_ORG; }
<OPCODE>\.?[cC][nN][oO][pP]		{ BEGIN(OPERAND); return T_CNOP; }
<OPCODE>\.?[eE][nN][dD][cC]		{ BEGIN(COMMENT); return T_ENDC; }
<FALSECLAUSE>\.?[eE][nN][dD][cC]	{ if(!--falseclause) { BEGIN(COMMENT); return T_ENDC; } }
<OPCODE>\.?[iI][fF][dD]			{ BEGIN(OPERAND); return T_IFD; }
<FALSECLAUSE>\.?[iI][fF][dD]		{ falseclause++; }
<OPCODE>\.?[iI][fF][nN][dD]		{ BEGIN(OPERAND); return T_IFND; }
<FALSECLAUSE>\.?[iI][fF][nN][dD]	{ falseclause++; }
<OPCODE>\.?[iI][fF][cC]			{ BEGIN(OPERAND); return T_IFC; }
<FALSECLAUSE>\.?[iI][fF][cC]		{ falseclause++; }
<OPCODE>\.?[iI][fF][nN][cC]		{ BEGIN(OPERAND); return T_IFNC; }
<FALSECLAUSE>\.?[iI][fF][nN][cC]	{ falseclause++; }
<OPCODE>\.?[iI][fF][eE][qQ]		{ BEGIN(OPERAND); return T_IFEQ; }
<FALSECLAUSE>\.?[iI][fF][eE][qQ]	{ falseclause++; }
<OPCODE>\.?[iI][fF][nN][eE]		{ BEGIN(OPERAND); return T_IFNE; }
<FALSECLAUSE>\.?[iI][fF][nN][eE]	{ falseclause++; }
<OPCODE>\.?[iI][fF][lL][tT]		{ BEGIN(OPERAND); return T_IFLT; }
<FALSECLAUSE>\.?[iI][fF][lL][tT]	{ falseclause++; }
<OPCODE>\.?[iI][fF][lL][eE]		{ BEGIN(OPERAND); return T_IFLE; }
<FALSECLAUSE>\.?[iI][fF][lL][eE]	{ falseclause++; }
<OPCODE>\.?[iI][fF][gG][tT]		{ BEGIN(OPERAND); return T_IFGT; }
<FALSECLAUSE>\.?[iI][fF][gG][tT]	{ falseclause++; }
<OPCODE>\.?[iI][fF][gG][eE]		{ BEGIN(OPERAND); return T_IFGE; }
<FALSECLAUSE>\.?[iI][fF][gG][eE]	{ falseclause++; }

<FALSECLAUSE>[^\n] { BEGIN(COMMENT); }

{number}		{yylval.num=strtol(yytext, NULL, 0); return T_NUMBER;}
{hex2}			{yylval.num=strtol(yytext+1, NULL, 16); return T_NUMBER;}
{binary}		{yylval.num=strtol(yytext+1, NULL, 2); return T_NUMBER;}


\<\<	return T_SHL;
\>\>	return T_SHR;
\<\=	return T_LE;
\>\=	return T_GE;
\=\=	return T_EQ;
\!\=	return T_NE;
\~	return T_COMPL;
\!	return T_NOT;
\+	return T_ADD;
\-	return T_SUB;
\*	return T_MUL;
\/	return T_DIV;
\%	return T_MOD;
\&	return T_AND;
\|	return T_OR;
\^	return T_XOR;
\<	return T_LT;
\>	return T_GT;
\=	return T_EQ;
\(	return T_LPAR;
\)	return T_RPAR;
\,	return T_COMMA;

<OPERAND>\#	return T_HASH;
<OPERAND>\@[Rr][0-3]	{yylval.num=yytext[2]&3; return T_ATR; }

<OPCODE>\.?[mM][aA][cC][rR][oO]	{ BEGIN(MACRO); return T_MACRO; }
<MACRO>{ws}+\.?[eE][nN][dD][mM]	{ BEGIN(OPCODE); yyless(((yytext[yyleng-5]=='.')?(yyleng-5):(yyleng-4))); yylval.str=poolstring(&macrotextpool, yytext); return T_MBODY; }
<MACRO>[^\n] { yymore(); BEGIN(MACRO2); }
<MACRO2>[^\n]+ { yymore(); }

(\"([^"\\]|\\.)*\")|(\'([^'\\]|\\.)*\')	{
	yylval.str=fixup_string(yytext);
	return T_STRING;
}

<OPCODE>{label}		{ if((yylval.sym=findmacro(yytext))) { BEGIN(MACARG); return T_MAC; } errormsg("Unknown opcode %s ignored", yytext); BEGIN(COMMENT); }
{label}				{ yylval.str=yytext; return T_SYMBOL; }

{ws}+	;
<MACRO,MACRO2>\n { current_lineno++; numlines++; yymore(); BEGIN(MACRO); }
<MACARG>\n	{ unput('\n'); BEGIN(INITIAL); return T_EMA; }
<*>\n	{ current_lineno++; numlines++; BEGIN(INITIAL); }
["']	{ errormsg("Unterminated string literal"); BEGIN(COMMENT); }
.	{ errormsg("Unknown character \"%s\"", yytext); BEGIN(COMMENT); }

%%

static char fixup_char(char **ptr, int n, int m, int b)
{
  int z;

  if(m<=0) return n;
  if(**ptr>='0' && **ptr<='9')
    z=**ptr&0xf;
  else if((**ptr>='A' && **ptr<='F') || (**ptr>='a' && **ptr<='f'))
    z=(**ptr&0xf)+9;
  else return n;
  if(z<b) {
    (*ptr)++;
    return fixup_char(ptr, n*b+z, m-1, b);
  }
  return n;
}

static char *fixup_string(char *ptr)
{
  char *dest=ptr, *src=ptr;
  char q=*src++;

  while(*src!=q)
    if((*dest++=*src++)=='\\')
      switch(dest[-1]=*src++) {
      case 'a': dest[-1]='\a'; break;
      case 'b': dest[-1]='\b'; break;
      case 'f': dest[-1]='\f'; break;
      case 'n': dest[-1]='\n'; break;
      case 'r': dest[-1]='\r'; break;
      case 't': dest[-1]='\t'; break;
      case 'v': dest[-1]='\v'; break;
      case 'x': dest[-1]=fixup_char(&src, 0, 2, 16); break;
      case '0': dest[-1]=fixup_char(&src, 0, 2, 8); break;
      case '1': dest[-1]=fixup_char(&src, 1, 2, 8); break;
      case '2': dest[-1]=fixup_char(&src, 2, 2, 8); break;
      case '3': dest[-1]=fixup_char(&src, 3, 2, 8); break;
      case '4': dest[-1]=fixup_char(&src, 4, 2, 8); break;
      case '5': dest[-1]=fixup_char(&src, 5, 2, 8); break;
      case '6': dest[-1]=fixup_char(&src, 6, 2, 8); break;
      case '7': dest[-1]=fixup_char(&src, 7, 2, 8); break;
      }
  *dest='\0';
  return ptr;
}

static char *fixup_macarg(char *ptr)
{
  int l;
  while(*ptr==' ' || *ptr=='\t') ptr++;
  for(l=strlen(ptr); l>0 && (ptr[l-1]==' ' || ptr[l-1]=='\t'); ptr[--l]='\0');
  return ptr;
}

void yyerror()
{
  errormsg("Parse error");
  BEGIN(COMMENT);
}

int yywrap()
{
  BEGIN(INITIAL);
  return file_yywrap();
}
