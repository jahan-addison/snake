
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taz.h"

#include "smach.h"
#include "backend.h"

SMV checknumU(SMV, int);
SMV checknumS(SMV, int);
SMV checknumW(SMV, int);
SMV checknumX(SMV, int);

extern int falseclause;
static struct symbol *label, *dlabel;
static int mac_argc;

extern struct mempool symbolnamepool;

#define MKICON(x)  mkicon((x))
#define MKLS1(x,y) mkls(M_LS1,(x),(y))
#define MKLS2(x,y) mkls(M_LS2,(x),(y))
#define MKADD(x,y) mkbinary(M_ADD,(x),(y))
#define MKSUB(x,y) mkbinary(M_SUB,(x),(y))
#define MKMUL(x,y) mkbinary(M_MUL,(x),(y))
#define MKDIV(x,y) mkbinary(M_DIV,(x),(y))
#define MKMOD(x,y) mkbinary(M_MOD,(x),(y))
#define MKAND(x,y) mkbinary(M_AND,(x),(y))
#define MKOR(x,y)  mkbinary(M_OR,(x),(y))
#define MKXOR(x,y) mkbinary(M_XOR,(x),(y))
#define MKSHL(x,y) mkbinary(M_SHL,(x),(y))
#define MKSHR(x,y) mkbinary(M_SHR,(x),(y))
#define MKNEG(x)   mkunary(M_NEG, (x))
#define MKCPL(x)   mkunary(M_CPL, (x))
#define MKNOT(x)   mkunary(M_NOT, (x))

%}

%union {
  numtype num;
  char *str;
  struct symbol *sym;
  SMV exp;
}

%token <str> T_LABEL T_SYMBOL
%token <num> T_NUMBER
%token <str> T_STRING
%token <str> T_MBODY T_MACARG
%token <sym> T_MAC

%token T_SECTION T_END
%token T_EQU T_SET T_INCLUDE T_INCDIR
%token T_MACRO T_ENDM T_MEXIT
%token T_ENDC T_IFD T_IFND T_IFC T_IFNC
%token T_IFEQ T_IFNE T_IFLT T_IFLE T_IFGT T_IFGE
%token T_EMA

%token T__ADD
%token T__AND
%token T__DIV
%token T__MUL
%token T__OR
%token T__SUB
%token T__XOR
%token T_ADDC
%token T_BE
%token T_BN
%token T_BNE
%token T_BNZ
%token T_BP
%token T_BPC
%token T_BR
%token T_BRF
%token T_BZ
%token T_CALL
%token T_CALLF
%token T_CALLR
%token T_CLR1
%token T_DBNZ
%token T_DEC
%token T_INC
%token T_JMP
%token T_JMPF
%token T_LD
%token T_LDC
%token T_MOV
%token T_NOP
%token T_NOT1
%token T_POP
%token T_PUSH
%token T_RET
%token T_RETI
%token T_ROL
%token T_ROLC
%token T_ROR
%token T_RORC
%token T_SET1
%token T_ST
%token T_SUBC
%token T_XCH
%token <num> T_ATR

%token T_BYTE
%token T_WORD
%token T_ORG
%token T_CNOP

%token T_SHL
%token T_SHR
%token T_LE
%token T_GE
%token T_EQ
%token T_NE
%token T_COMPL
%token T_NOT
%token T_ADD
%token T_SUB
%token T_MUL
%token T_DIV
%token T_MOD
%token T_AND
%token T_OR
%token T_XOR
%token T_LT
%token T_GT
%token T_EQ
%token T_LPAR
%token T_RPAR
%token T_COMMA
%token T_HASH

%left  T_OR
%left  T_XOR
%left  T_AND
%nonassoc T_EQ T_NE
%nonassoc T_LT T_LE T_GT T_GE
%left  T_SHL T_SHR
%left  T_ADD T_SUB
%left  T_MUL T_DIV T_MOD
%right T_COMPL T_NOT

%type <str> section_name section_qualifier string
%type <num> conditional const_expr
%type <exp> expr

%type <exp> b3
%type <exp> i8
%type <exp> d9
%type <exp> a12
%type <exp> a16
%type <num> r8s
%type <exp> r8l
%type <exp> r16

%%

program : headers units opt_end

units : implicit_section
      | explicit_sections
      |

explicit_sections : explicit_section
		  | explicit_sections explicit_section

explicit_section : section_header statements

section_header : T_SECTION section_name
	       | T_SECTION section_name T_COMMA section_qualifier

section_name : T_SYMBOL

section_qualifier : T_SYMBOL

opt_end : T_END |

implicit_section : statement0
		 | implicit_section statement

headers : headers header
	|

statement0 : pseudo
	   | opcode
	   | T_LABEL { if(!(label=create_symbol($1, SYMB_RELOC))) YYERROR;
                       else { label->value.num=0xdeadbeef;
		              smach_setsym(label, mksymbref(currloc_sym)); } }

statement : statement0
          | header

statements : statements statement
           |

header : error
       | T_LABEL T_EQU
		{ if(!(dlabel=create_symbol($1, SYMB_EQU))) YYERROR; }
	 expr
		{ smach_setsym(dlabel, $4); }
       | T_LABEL T_EQ
		{ if(!(dlabel=create_symbol($1, SYMB_EQU))) YYERROR; }
	 expr
		{ smach_setsym(dlabel, $4); }
       | T_LABEL T_SET
		{ if(!(dlabel=create_symbol($1, SYMB_SET))) YYERROR; }
	 const_expr
		{ smach_setsym(dlabel, $4); }
       | T_INCDIR T_STRING
		{ add_incdir($2); }
       | T_INCLUDE T_STRING
		{ if(!process_include($2))
			errormsg("Can't find include file %s", $2); }
       | conditional { if(!$1) falseclause=1;  } cond_block T_ENDC
	{ if(!$1) falseclause=0; }
       | T_LABEL T_MACRO 
		{ if(!(label=create_symbol($1, SYMB_MACRO))) YYERROR; }
	 T_MBODY T_ENDM
		{ label->value.macro=$4; }
       | T_MAC { mac_argc=0; } macargs_opt T_EMA
		{ invoke_macro($1, mac_argc); }
       | T_MEXIT { if(!abort_macro()) { errormsg("Spurious mexit"); YYERROR; }}
       | T_ENDC { errormsg("Spurious endc"); YYERROR; }
       | T_ENDM { errormsg("Spurious endm"); YYERROR; }

pseudo
       : T_BYTE expr_list_8
       | T_WORD expr_list_16
       | T_ORG expr { smach_setsym(currloc_sym, $2); }
       | T_CNOP const_expr T_COMMA const_expr { smach_cnop($2, $4); }

cond_block : cond_block statement
	   | 

conditional
	: T_IFD T_SYMBOL { $$ = !!lookup_symbol($2); }
	| T_IFND T_SYMBOL { $$ = !lookup_symbol($2); }
	| T_IFC string T_COMMA T_STRING { $$ = (strcmp($2, $4) == 0); }
	| T_IFNC string T_COMMA T_STRING { $$ = (strcmp($2, $4) != 0); }
	| T_IFEQ const_expr T_COMMA const_expr { $$ = $2==$4; }
	| T_IFNE const_expr T_COMMA const_expr { $$ = $2!=$4; }
	| T_IFLT const_expr T_COMMA const_expr { $$ = $2<$4; }
	| T_IFLE const_expr T_COMMA const_expr { $$ = $2<=$4; }
	| T_IFGT const_expr T_COMMA const_expr { $$ = $2>$4; }
	| T_IFGE const_expr T_COMMA const_expr { $$ = $2>=$4; }
	| T_IFEQ const_expr { $$ = $2==0; }
	| T_IFNE const_expr { $$ = $2!=0; }
	| T_IFLT const_expr { $$ = $2<0; }
	| T_IFLE const_expr { $$ = $2<=0; }
	| T_IFGT const_expr { $$ = $2>0; }
	| T_IFGE const_expr { $$ = $2>=0; }

macargs_opt
	: macargs
	|

macargs
	: macarg
	| macargs T_COMMA macarg

macarg	: T_MACARG
	{ macro_storearg(mac_argc++, $1); }


opcode
 : T__ADD T_HASH i8
	{ GEN(MKOR(MKICON(33024),$3),16); }
 | T__ADD d9
	{ GEN(MKOR(MKICON(33280),$2),16); }
 | T__ADD T_ATR
	{ GEN(MKICON(132|($2)),8); }
 | T_ADDC T_HASH i8
	{ GEN(MKOR(MKICON(37120),$3),16); }
 | T_ADDC d9
	{ GEN(MKOR(MKICON(37376),$2),16); }
 | T_ADDC T_ATR
	{ GEN(MKICON(148|($2)),8); }
 | T__SUB T_HASH i8
	{ GEN(MKOR(MKICON(41216),$3),16); }
 | T__SUB d9
	{ GEN(MKOR(MKICON(41472),$2),16); }
 | T__SUB T_ATR
	{ GEN(MKICON(164|($2)),8); }
 | T_SUBC T_HASH i8
	{ GEN(MKOR(MKICON(45312),$3),16); }
 | T_SUBC d9
	{ GEN(MKOR(MKICON(45568),$2),16); }
 | T_SUBC T_ATR
	{ GEN(MKICON(180|($2)),8); }
 | T_INC d9
	{ GEN(MKOR(MKICON(25088),$2),16); }
 | T_INC T_ATR
	{ GEN(MKICON(100|($2)),8); }
 | T_DEC d9
	{ GEN(MKOR(MKICON(29184),$2),16); }
 | T_DEC T_ATR
	{ GEN(MKICON(116|($2)),8); }
 | T__MUL
	{ GEN(MKICON(48),8); }
 | T__DIV
	{ GEN(MKICON(64),8); }
 | T__AND T_HASH i8
	{ GEN(MKOR(MKICON(57600),$3),16); }
 | T__AND d9
	{ GEN(MKOR(MKICON(57856),$2),16); }
 | T__AND T_ATR
	{ GEN(MKICON(228|($2)),8); }
 | T__OR T_HASH i8
	{ GEN(MKOR(MKICON(53504),$3),16); }
 | T__OR d9
	{ GEN(MKOR(MKICON(53760),$2),16); }
 | T__OR T_ATR
	{ GEN(MKICON(212|($2)),8); }
 | T__XOR T_HASH i8
	{ GEN(MKOR(MKICON(61696),$3),16); }
 | T__XOR d9
	{ GEN(MKOR(MKICON(61952),$2),16); }
 | T__XOR T_ATR
	{ GEN(MKICON(244|($2)),8); }
 | T_ROL
	{ GEN(MKICON(224),8); }
 | T_ROLC
	{ GEN(MKICON(240),8); }
 | T_ROR
	{ GEN(MKICON(192),8); }
 | T_RORC
	{ GEN(MKICON(208),8); }
 | T_LD d9
	{ GEN(MKOR(MKICON(512),$2),16); }
 | T_LD T_ATR
	{ GEN(MKICON(4|($2)),8); }
 | T_ST d9
	{ GEN(MKOR(MKICON(4608),$2),16); }
 | T_ST T_ATR
	{ GEN(MKICON(20|($2)),8); }
 | T_MOV T_HASH i8 T_COMMA d9
	{ GEN(MKOR(MKOR(MKICON(2228224),MKLS1($5,8)),$3),24); }
 | T_MOV T_HASH i8 T_COMMA T_ATR
	{ GEN(MKOR(MKICON(9216|(($5)<<8)),$3),16); }
 | T_LDC
	{ GEN(MKICON(193),8); }
 | T_PUSH d9
	{ GEN(MKOR(MKICON(24576),$2),16); }
 | T_POP d9
	{ GEN(MKOR(MKICON(28672),$2),16); }
 | T_XCH d9
	{ GEN(MKOR(MKICON(49664),$2),16); }
 | T_XCH T_ATR
	{ GEN(MKICON(196|($2)),8); }
 | T_JMP a12
	{ GEN(MKOR(MKICON(10240),MKOR(MKLS1(MKAND(MKICON(2048),$2),1),MKAND(MKICON(2047),$2))),16); }
 | T_JMPF a16
	{ GEN(MKOR(MKICON(2162688),$2),24); }
 | T_BR r8s
	{ GEN(MKOR(MKICON(256),$2),16); }
 | T_BRF r16
	{ GEN(MKOR(MKICON(1114112),$2),24); }
 | T_BZ r8s
	{ GEN(MKOR(MKICON(32768),$2),16); }
 | T_BNZ r8s
	{ GEN(MKOR(MKICON(36864),$2),16); }
 | T_BP d9 T_COMMA b3 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKOR(MKICON(6815744),MKOR(MKLS1(MKAND(MKICON(256),$2),12),MKLS1(MKAND(MKICON(255),$2),8))),MKLS1($4,16)),$6),24); }
 | T_BPC d9 T_COMMA b3 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKOR(MKICON(4718592),MKOR(MKLS1(MKAND(MKICON(256),$2),12),MKLS1(MKAND(MKICON(255),$2),8))),MKLS1($4,16)),$6),24); }
 | T_BN d9 T_COMMA b3 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKOR(MKICON(8912896),MKOR(MKLS1(MKAND(MKICON(256),$2),12),MKLS1(MKAND(MKICON(255),$2),8))),MKLS1($4,16)),$6),24); }
 | T_DBNZ d9 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKICON(5373952),MKLS1($2,8)),$4),24); }
 | T_DBNZ T_ATR T_COMMA r8s
	{ GEN(MKOR(MKICON(21504|(($2)<<8)),$4),16); }
 | T_BE T_HASH i8 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKICON(3211264),MKLS1($3,8)),$5),24); }
 | T_BE d9 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKICON(3276800),MKLS1($2,8)),$4),24); }
 | T_BE T_ATR T_COMMA T_HASH i8 T_COMMA r8l
	{ GEN(MKOR(MKOR(MKICON(3407872|(($2)<<16)),MKLS1($5,8)),$7),24); }
 | T_BNE T_HASH i8 T_COMMA r8l
        { GEN(MKOR(MKOR(MKICON(4259840),MKLS1($3,8)),$5),24); }
 | T_BNE d9 T_COMMA r8l
        { GEN(MKOR(MKOR(MKICON(4325376),MKLS1($2,8)),$4),24); }
 | T_BNE T_ATR T_COMMA T_HASH i8 T_COMMA r8l
        { GEN(MKOR(MKOR(MKICON(4456448|(($2)<<16)),MKLS1($5,8)),$7),24); }
 | T_CALL a12
	{ GEN(MKOR(MKICON(2048),MKOR(MKLS1(MKAND(MKICON(2048),$2),1),MKAND(MKICON(2047),$2))),16); }
 | T_CALLF a16
	{ GEN(MKOR(MKICON(2097152),$2),24); }
 | T_CALLR r16
	{ GEN(MKOR(MKICON(1048576),$2),24); }
 | T_RET
	{ GEN(MKICON(160),8); }
 | T_RETI
	{ GEN(MKICON(176),8); }
 | T_CLR1 d9 T_COMMA b3
	{ GEN(MKOR(MKOR(MKICON(51200),MKOR(MKLS1(MKAND(MKICON(256),$2),4),MKAND(MKICON(255),$2))),MKLS1($4,8)),16); }
 | T_SET1 d9 T_COMMA b3
	{ GEN(MKOR(MKOR(MKICON(59392),MKOR(MKLS1(MKAND(MKICON(256),$2),4),MKAND(MKICON(255),$2))),MKLS1($4,8)),16); }
 | T_NOT1 d9 T_COMMA b3
	{ GEN(MKOR(MKOR(MKICON(43008),MKOR(MKLS1(MKAND(MKICON(256),$2),4),MKAND(MKICON(255),$2))),MKLS1($4,8)),16); }
 | T_NOP
	{ GEN(MKICON(0),8); }


b3 : expr
  { $$ = checknumU($1, 3); }

i8 : expr
  { $$ = checknumX($1, 8); }

d9 : expr
  { $$ = checknumU($1, 9); }

a12 : expr
  { $$ = checknumU(MKXOR($1, MKAND(MKICON(-4096), MKADD(mksymbref(currloc_sym), MKICON(2)))), 12); }

a16 : expr
  { $$ = checknumU($1, 16); }

r8s : expr
  { $$ = checknumS(MKSUB($1, MKADD(mksymbref(currloc_sym), MKICON(2))), 8); }

r8l : expr
  { $$ = checknumS(MKSUB($1, MKADD(mksymbref(currloc_sym), MKICON(3))), 8); }

r16 : expr
  { SMV t = MKSUB(checknumU($1, 16), MKADD(mksymbref(currloc_sym), MKICON(2))); $$ = MKOR(MKSHR(MKAND(MKICON(65280), t), MKICON(8)), MKSHL(MKAND(MKICON(255), t), MKICON(8))); }

expr_list_8 : expr_8
            | expr_list_8 T_COMMA expr_8

expr_8 : expr { GEN(checknumX($1, 8), 8); }
       | T_STRING { { int i; for(i=0; ($1)[i]; i++) GEN(MKICON(($1)[i]), 8);} }

expr_list_16 : expr_16
             | expr_list_16 T_COMMA expr_16

expr_16 : expr
  { numtype n; SMV t = checknumX($1, 16); if(evalconst(t, &n)) GEN(MKICON(((n&0xff)<<8)|((n&0xff00)>>8)), 16); else { HOLD(MKSHR(t, MKICON(8)), 8); GEN(MKAND(t, MKICON(255)), 8); } }


const_expr : expr { numtype n; if(evalconst($1, &n)) $$ = n; else { errormsg("Expression does not compute to a constant value"); YYERROR; } }

expr : T_LPAR expr T_RPAR { $$ = $2; }
     | T_NUMBER { $$ = MKICON($1); }
     | T_STRING { if(strlen($1)!=1) { errormsg("Character expression has length != 1"); YYERROR; } else { $$ = MKICON($1[0]); } }
     | T_SYMBOL
	{ if((label=lookup_symbol($1))) $$=mksymbref(label); else if((label=create_symbol(poolstring(&symbolnamepool, $1), SYMB_UNDEF))) $$=mksymbref(label); else YYERROR; }
     | T_MUL { $$=mksymbref(currloc_sym); }
     | T_LT expr %prec T_NOT { $$ = MKAND($2, MKICON(255)); }
     | T_GT expr %prec T_NOT { $$ = MKSHR($2, MKICON(8)); }
     | T_SUB expr %prec T_NOT { $$ = MKNEG($2); }
     | T_ADD expr %prec T_NOT { $$ = $2; }
     | T_COMPL expr { $$ = MKCPL($2); }
     | T_NOT expr { $$ = MKNOT($2); }
     | expr T_ADD expr { $$ = MKADD($1,$3); }
     | expr T_SUB expr { $$ = MKSUB($1,$3); }
     | expr T_MUL expr { $$ = MKMUL($1,$3); }
     | expr T_DIV expr { $$ = MKDIV($1,$3); }
     | expr T_MOD expr { $$ = MKMOD($1,$3); }
     | expr T_AND expr { $$ = MKAND($1,$3); }
     | expr T_OR  expr { $$ = MKOR($1,$3); }
     | expr T_NOT expr %prec T_OR { $$ = MKOR($1,$3); }
     | expr T_XOR expr { $$ = MKXOR($1,$3); }
     | expr T_SHL expr { $$ = MKSHL($1,$3); }
     | expr T_SHR expr { $$ = MKSHR($1,$3); }

string : T_STRING
	{ $$ = poolstring(&symbolnamepool, $1); }

%%

SMV checknumU(SMV val, int bits)
{
  return mkchecknum(M_CHECKU, val, bits);
}

SMV checknumS(SMV val, int bits)
{
  return mkchecknum(M_CHECKS, val, bits);
}

SMV checknumW(SMV val, int bits)
{
  return mkchecknum(M_CHECKW, val, bits);
}

SMV checknumX(SMV val, int bits)
{
  return mkchecknum(M_CHECKX, val, bits);
}

