#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taz.h"
#include "smach.h"
#include "backend.h"

union smval {
  int type;
  struct {
    int type;
    numtype num;
  } number;
  struct {
    int type;
    struct symbol *symb;
  } symbol;
  struct {
    int type;
    SMV v;
  } unary;
  struct {
    int type;
    SMV v1, v2;
  } binary;
  struct {
    int type;
    SMV v;
    int n;
  } ls;
};

static struct symbol *changed;

#define OP_PREFIX(n) (n)
#define OP_PUSH(n) (0x80|(n))
#define OP_PUSHN(n) (0xa0|(n))
#define OP_LOADL (0xc0)
#define OP_STOREL (0xc1)
#define OP_BREAK (0xc2)
#define OP_ARITH(n) (0xc3+(n)-M_NEG)
#define OP_LS1(n) (0xd0|((n)-1))
#define OP_LS2(n) (0xe0|(((n)>>4)-1))
/* #define OP_STPREFIX(n) (0xe8|(n)) */
#define OP_ORG    0xe9
#define OP_CNOP   0xea
#define OP_XARITH 0xeb
#define OP_CHECKU 0xec
#define OP_CHECKS 0xed
#define OP_CHECKW 0xee
#define OP_CHECKX 0xef
#define OP_STINLINE(n) (0xf0|((n)>>3))
#define OP_STORE(n) (0xf8|((n)>>3))
#define OP_STOREPB (0xe8)
#define OP_STINLINEPB (0xf0)
#define OP_LINENO (0xe0)
#define OP_FILENAME (0xf8)

static union smval *SMVs;
static int numSMVs, maxSMVs;

#define SMVelt(s,f) (SMVs[s].f)

static unsigned char *emit_p, *emit_brk;
static unsigned char **code_blk;
static int regd_blks, allocated_blks;

#define CODEBLKSZ (0x10000)

#define EMIT(b) ((emit_p+1<emit_brk?(void)(*emit_p++=(b)):femit(b)))
#define ALLOT(b,n) ((emit_p+(n)+1<emit_brk?(void)0:extend_emit((n)+1)),*emit_p++=(b),(emit_p+=(n)))

#define STKVAL0(n) (valstack[n])
#define STKVAL(n) (STKVAL0(stkp-(n)-1))
#define PUSH(n) (STKVAL0(stkp++)=(n))
#define POP (STKVAL0(--stkp))

static int stkp=0, saved_lineno;
static numtype valstack[100];

static SMV allocSMV()
{
  if(numSMVs>=maxSMVs) {
    if(SMVs!=NULL)
      SMVs=realloc(SMVs, (maxSMVs<<=1)*sizeof(union smval));
    else
      SMVs=malloc((maxSMVs=64)*sizeof(union smval));
    if(SMVs==NULL) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }
  }
  return numSMVs++;
}

SMV mkicon(numtype n)
{
  SMV s=allocSMV();
  SMVelt(s,type)=M_ICON;
  SMVelt(s,number.num)=n;
  return s;
}

static SMV divzeroSMV()
{
  errormsg("Division by zero");
  return mkicon(1);
}

SMV mksymbref(struct symbol *sy)
{
  if(sy->type == SYMB_EQU || sy->type == SYMB_SET)
    return mkicon(sy->value.num);
  else {
    SMV s=allocSMV();
    SMVelt(s,type)=M_SYMB;
    SMVelt(s,symbol.symb)=sy;
    return s;
  }
}

SMV mkchecknum(int t, SMV v, int b)
{
  if(SMVelt(v,type)==M_ICON) {
    switch(t) {
     case M_CHECKU:
       if(SMVelt(v,number.num)<0 || SMVelt(v,number.num)>=(1<<b)) {
	 errormsg("value is out of range %d bits unsigned", b);
	 return mkicon(0);
       }
       break;
     case M_CHECKS:
       if(SMVelt(v,number.num)<-(1<<(b-1)) ||
	  SMVelt(v,number.num)>=(1<<(b-1))) {
	 errormsg("value is out of range %d bits signed", b);
	 return mkicon(0);
       }
       break;
     case M_CHECKW:
       if(SMVelt(v,number.num)<1 || SMVelt(v,number.num)>(1<<b)) {
	 errormsg("value is out of range %d bits wraparound", b);
	 return mkicon(0);
       }
       break;
     case M_CHECKX:
       if(SMVelt(v,number.num)<-(1<<b) || SMVelt(v,number.num)>=(1<<b)) {
	 errormsg("value is out of range %d bits", b);
	 return mkicon(0);
       }
       break;
    }
    SMVelt(v,number.num)=SMVelt(v,number.num)&((1<<b)-1);
    return v;
  }
  else {
    SMV s=allocSMV();
    SMVelt(s,type)=t;
    SMVelt(s,ls.v)=v;
    SMVelt(s,ls.n)=b;
    return s;
  }
}

SMV mkunary(int t, SMV v)
{
  if(SMVelt(v,type)==M_ICON) {
    numtype n=SMVelt(v,number.num);
    switch(t) {
    case M_NEG: n=-n; break;
    case M_CPL: n=~n; break;
    case M_NOT: n=!n; break;
    }
    SMVelt(v,number.num)=n;
    return v;
  } else {
    SMV s=allocSMV();
    SMVelt(s,type)=t;
    SMVelt(s,unary.v)=v;
    return s;
  }
}

SMV mkbinary(int t, SMV v1, SMV v2)
{
  if(SMVelt(v1,type)==M_ICON && SMVelt(v2,type)==M_ICON) {
    numtype n2=SMVelt(v2,number.num);
    switch(t) {
    case M_ADD: SMVelt(v1,number.num)+=n2; break;
    case M_SUB: SMVelt(v1,number.num)-=n2; break;
    case M_MUL: SMVelt(v1,number.num)*=n2; break;
    case M_DIV: if(!n2) return divzeroSMV(); SMVelt(v1,number.num)/=n2; break;
    case M_MOD: if(!n2) return divzeroSMV(); SMVelt(v1,number.num)%=n2; break;
    case M_AND: SMVelt(v1,number.num)&=n2; break;
    case M_OR: SMVelt(v1,number.num)|=n2; break;
    case M_XOR: SMVelt(v1,number.num)^=n2; break;
    case M_SHL: SMVelt(v1,number.num)<<=n2; break;
    case M_SHR: SMVelt(v1,number.num)>>=n2; break;
    }
    return v1;
  } else {
    SMV s=allocSMV();
    SMVelt(s,type)=t;
    SMVelt(s,binary.v1)=v1;
    SMVelt(s,binary.v2)=v2;
    return s;
  }
}

SMV mkls(int t, SMV v, int n)
{
  if(SMVelt(v,type)==M_ICON) {
    SMVelt(v,number.num)<<=n;
    return v;
  } else {
    SMV s=allocSMV();
    SMVelt(s,type)=t;
    SMVelt(s,ls.v)=v;
    SMVelt(s,ls.n)=n;
    return s;
  }
}

int evalconst(SMV s, numtype *n)
{
  if(SMVelt(s,type)==M_ICON) {
    *n=SMVelt(s,number.num);
    return TRUE;
  } else if(SMVelt(s,type)==M_SYMB) {
    struct symbol *sy=SMVelt(s,symbol.symb);
    if(sy->type==SYMB_EQU || sy->type==SYMB_SET) {
      *n=sy->value.num;
      return TRUE;
    } else return FALSE;
  } else if(SMVelt(s,type)<=M_MAX_UNARY) {
    numtype n1;
    if(evalconst(SMVelt(s,unary.v),&n1)) {
      switch(SMVelt(s,type)) {
      case M_NEG: *n=-n1;
      case M_CPL: *n=~n1;
      case M_NOT: *n=!n;
      }
      return TRUE;
    } else return FALSE;
  } else if(SMVelt(s,type)<=M_MAX_BINARY) {
    numtype n1, n2;
    if(evalconst(SMVelt(s,binary.v1),&n1) &&
       evalconst(SMVelt(s,binary.v2),&n2)) {
      switch(SMVelt(s,type)) {
      case M_ADD: n1+=n2; break;
      case M_SUB: n1-=n2; break;
      case M_MUL: n1*=n2; break;
      case M_DIV: if(!n2) return FALSE; n1/=n2; break;
      case M_MOD: if(!n2) return FALSE; n1%=n2; break;
      case M_AND: n1&=n2; break;
      case M_OR: n1|=n2; break;
      case M_XOR: n1^=n2; break;
      case M_SHL: n1<<=n2; break;
      case M_SHR: n1>>=n2; break;
      }
      *n=n1;
      return TRUE;
    } else return FALSE;
  } else return FALSE;
}

void smach_flush()
{
  numSMVs = 0;
}

static void register_codeblk(unsigned char *p)
{
  if(regd_blks>=allocated_blks) {
    if(allocated_blks != 0) {
      allocated_blks <<= 1;
      code_blk = realloc(code_blk, allocated_blks*sizeof(unsigned char **));
    } else {
      allocated_blks = 10;
      code_blk = malloc(allocated_blks*sizeof(unsigned char **));
    }
    if(code_blk == NULL) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }
  }
  code_blk[regd_blks++] = p;
}

static void extend_emit(int n)
{
  if(emit_p)
    *emit_p = OP_BREAK;
  if(++n<CODEBLKSZ)
    n=CODEBLKSZ;
  emit_p = malloc(n);
  emit_brk = emit_p + n;
  if(emit_p == NULL) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  register_codeblk(emit_p);
}

static void femit(int b)
{
  extend_emit(1);
  *emit_p++ = b;
}
 
void smach_dump_binary(FILE *f, SMV v, char *o)
{
  void smach_dump(FILE *f, SMV v);

  fprintf(f, "(");
  smach_dump(f, SMVelt(v, binary).v1);
  fprintf(f, "%s", o);
  smach_dump(f, SMVelt(v, binary).v2);
  fprintf(f, ")");
}

void smach_dump(FILE *f, SMV v)
{
  switch(SMVelt(v,type)) {
  case M_ICON: fprintf(f, "%ld", SMVelt(v, number).num); break;
  case M_SYMB: fprintf(f, "%s", SMVelt(v, symbol).symb->name); break;
  case M_NEG: fprintf(f, "-"); smach_dump(f, SMVelt(v, unary).v); break;
  case M_CPL: fprintf(f, "~"); smach_dump(f, SMVelt(v, unary).v); break;
  case M_NOT: fprintf(f, "!"); smach_dump(f, SMVelt(v, unary).v); break;
  case M_ADD: smach_dump_binary(f, v, "+"); break;
  case M_SUB: smach_dump_binary(f, v, "-"); break;
  case M_MUL: smach_dump_binary(f, v, "*"); break;
  case M_DIV: smach_dump_binary(f, v, "/"); break;
  case M_MOD: smach_dump_binary(f, v, "%"); break;
  case M_AND: smach_dump_binary(f, v, "&"); break;
  case M_OR: smach_dump_binary(f, v, "|"); break;
  case M_XOR: smach_dump_binary(f, v, "^"); break;
  case M_SHL: smach_dump_binary(f, v, "<<"); break;
  case M_SHR: smach_dump_binary(f, v, ">>"); break;
  case M_LS1:
  case M_LS2:
    smach_dump(f, SMVelt(v, ls).v); fprintf(f, "<<%d", SMVelt(v, ls).n); break;
  case M_CHECKU:
    fprintf(f, "U%d(", SMVelt(v, ls).n); smach_dump(f, SMVelt(v, ls).v);
    fprintf(f, ")"); break;
  case M_CHECKS:
    fprintf(f, "S%d(", SMVelt(v, ls).n); smach_dump(f, SMVelt(v, ls).v);
    fprintf(f, ")"); break;
  case M_CHECKW:
    fprintf(f, "W%d(", SMVelt(v, ls).n); smach_dump(f, SMVelt(v, ls).v);
    fprintf(f, ")"); break;
  case M_CHECKX:
    fprintf(f, "X%d(", SMVelt(v, ls).n); smach_dump(f, SMVelt(v, ls).v);
    fprintf(f, ")"); break;
  default:
    fprintf(f, "?%d?", SMVelt(v,type));
  }
}

static void emit_prefix(numtype n)
{
  unsigned char s[sizeof(numtype)*2];
  int z=0;

  if(!n)
    return;
  while(n>0x7f) {
    s[z++] = (n&0x7f);
    n >>= 7;
  }
  EMIT(OP_PREFIX(n));
  while(z--)
    EMIT(OP_PREFIX(s[z]));
}

static void emit_symprefix(struct symbol *s)
{
  emit_prefix((numtype)(void*)s);
}

static void emit_inline(numtype n, int b)
{
  unsigned char *p;
  for(p=emit_p; b--;) {
    *--p = n&0xff;
    n >>= 8;
  }
}

static void emit(SMV v)
{
  if(SMVelt(v,type)>=M_NEG && SMVelt(v,type)<=M_MAX_BINARY) {
    if(SMVelt(v,type)>M_MAX_UNARY) {
      emit(SMVelt(v,binary).v1);
      emit(SMVelt(v,binary).v2);
    } else
      emit(SMVelt(v,unary).v);
    EMIT(OP_ARITH(SMVelt(v,type)));
  } else switch(SMVelt(v,type)) {
  case M_ICON:
    {
      numtype n = SMVelt(v,number).num;
      if(n<0) {
	n = ~n;
	if(n<32)
	  EMIT(OP_PUSHN(n));
	else {
	  emit_prefix(n>>5);
	  EMIT(OP_PUSHN(n&31));
	}
      } else {
	if(n<32)
	  EMIT(OP_PUSH(n));
	else {
	  emit_prefix(n>>5);
	  EMIT(OP_PUSH(n&31));
	}	
      }
    }
    break;
  case M_SYMB:
    emit_symprefix(SMVelt(v,symbol).symb);
    EMIT(OP_LOADL);
    break;
  case M_LS1:
    emit(SMVelt(v,ls).v);
    EMIT(OP_LS1(SMVelt(v,ls).n));
    break;
  case M_LS2:
    emit(SMVelt(v,ls).v);
    EMIT(OP_LS2(SMVelt(v,ls).n));
    break;
  case M_CHECKU:
    emit(SMVelt(v,ls).v);
    emit_prefix(SMVelt(v,ls).n);
    EMIT(OP_CHECKU);
    break;
   case M_CHECKS:
    emit(SMVelt(v,ls).v);
    emit_prefix(SMVelt(v,ls).n);
    EMIT(OP_CHECKS);
    break;
  case M_CHECKW:
    emit(SMVelt(v,ls).v);
    emit_prefix(SMVelt(v,ls).n);
    EMIT(OP_CHECKW);
    break;
  case M_CHECKX:
    emit(SMVelt(v,ls).v);
    emit_prefix(SMVelt(v,ls).n);
    EMIT(OP_CHECKX);
    break;
  default:
    fprintf(stderr, "Internal error: smach:emit(%d)\n", SMVelt(v,type));
    exit(3);
  }
}

void smach_pushfile(int id)
{
  if(current_lineno > saved_lineno) {
    emit_prefix(current_lineno-saved_lineno-1);
    EMIT(OP_LINENO);
    saved_lineno = current_lineno;
  }
  emit_prefix(id+1);
  EMIT(OP_FILENAME);
  saved_lineno = 0;
}

void smach_popfile()
{
  EMIT(OP_FILENAME);
  saved_lineno = current_lineno;
}

void smach_emit(SMV v, int b)
{
  /*
    fprintf(stderr, "EMIT%d ", b);
    smach_dump(stderr, v);
    fprintf(stderr, "\n");
  */
  if(current_lineno > saved_lineno) {
    emit_prefix(current_lineno-saved_lineno-1);
    EMIT(OP_LINENO);
    saved_lineno = current_lineno;
  }
  if(SMVelt(v,type)==M_ICON) {
    /*  if(b<8) {
     if(SMVelt(v,number).num&0x7f)
       EMIT(OP_PREFIX(SMVelt(v,number).num&0x7f));
     EMIT(OP_STPREFIX(b));
     } else*/ if((b&7) || b>=64) {
      emit_prefix(b);
      ALLOT(OP_STINLINEPB, (b+7)>>3);
      emit_inline(SMVelt(v,number).num, (b+7)>>3);
    } else {
      ALLOT(OP_STINLINE(b), b>>3);
      emit_inline(SMVelt(v,number).num, b>>3);
    }
  } else {
    emit(v);
    if((b&7) || b>=64) {
      emit_prefix(b);
      EMIT(OP_STOREPB);
    } else
      EMIT(OP_STORE(b));
  }
}

void smach_setsym(struct symbol *s, SMV v)
{
  numtype n;

  if(s == currloc_sym) {
    emit(v);
    EMIT(OP_ORG);
  } else if(evalconst(v, &n))
    s->value.num = n;
  else {
    s->type = (s->type==SYMB_SET? SYMB_UNDSET : SYMB_UNDECIDED);
    emit(v);
    emit_symprefix(s);
    EMIT(OP_STOREL);
  }
}

void smach_cnop(numtype offs, numtype modulo)
{
  if(modulo<=0 || (modulo & (modulo - 1)) != 0)
    errormsg("second argument to cnop must be a positive power of two");
  else if(offs<0 || offs>=modulo)
    errormsg("first argument to cnop out of range");
  else {
    emit_prefix(modulo | offs);
    EMIT(OP_CNOP);
  }
}

void smach_term()
{
  if(emit_p)
    *emit_p = OP_BREAK;
}

static struct symbol *locate_sym(numtype n)
{
  return (struct symbol *)(void *)n;
}

static numtype findmsb(numtype n)
{
  numtype b=1;
  while(b<=n) b<<=1;
  return b>>1;
}

static numtype smach_execute(unsigned char *p, numtype p0)
{
  numtype pfx=p0;

  for(;;) {
    int o=*p++;
/*
int i;
fprintf(stderr, "%02x:", o);
for(i=0; i<stkp; i++) fprintf(stderr, " %lx", STKVAL0(i));
fprintf(stderr, "\n");
*/
    if(o&0x80) {
      if(o&0x40) switch(o) {
      case OP_BREAK:
	return pfx;
      case OP_ARITH(M_NEG): STKVAL(0)=-STKVAL(0); break;
      case OP_ARITH(M_CPL): STKVAL(0)=~STKVAL(0); break;
      case OP_ARITH(M_NOT): STKVAL(0)=!STKVAL(0); break;
      case OP_ARITH(M_ADD): STKVAL(1)+=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_SUB): STKVAL(1)-=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_MUL): STKVAL(1)*=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_DIV): if(STKVAL(0)) STKVAL(1)/=STKVAL(0); else errormsg("Division by zero"); (void)POP; break;
      case OP_ARITH(M_MOD): if(STKVAL(0)) STKVAL(1)%=STKVAL(0); else errormsg("Division by zero"); (void)POP; break;
      case OP_ARITH(M_AND): STKVAL(1)&=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_OR): STKVAL(1)|=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_XOR): STKVAL(1)^=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_SHL): STKVAL(1)<<=STKVAL(0); (void)POP; break;
      case OP_ARITH(M_SHR): STKVAL(1)>>=STKVAL(0); (void)POP; break;
      case OP_LS1(1): STKVAL(0)<<=1; break;
      case OP_LS1(2): STKVAL(0)<<=2; break;
      case OP_LS1(3): STKVAL(0)<<=3; break;
      case OP_LS1(4): STKVAL(0)<<=4; break;
      case OP_LS1(5): STKVAL(0)<<=5; break;
      case OP_LS1(6): STKVAL(0)<<=6; break;
      case OP_LS1(7): STKVAL(0)<<=7; break;
      case OP_LS1(8): STKVAL(0)<<=8; break;
      case OP_LS1(9): STKVAL(0)<<=9; break;
      case OP_LS1(10): STKVAL(0)<<=10; break;
      case OP_LS1(11): STKVAL(0)<<=11; break;
      case OP_LS1(12): STKVAL(0)<<=12; break;
      case OP_LS1(13): STKVAL(0)<<=13; break;
      case OP_LS1(14): STKVAL(0)<<=14; break;
      case OP_LS1(15): STKVAL(0)<<=15; break;
      case OP_LS1(16): STKVAL(0)<<=16; break;
      case OP_LS2(32): STKVAL(0)<<=32; break;
      case OP_LS2(48): STKVAL(0)<<=48; break;
      case OP_LS2(64): STKVAL(0)<<=64; break;
      case OP_LS2(80): STKVAL(0)<<=80; break;
      case OP_LS2(96): STKVAL(0)<<=96; break;
      case OP_LS2(112): STKVAL(0)<<=112; break;
      case OP_LS2(128): STKVAL(0)<<=128; break;
      case OP_CHECKU:
	if(STKVAL(0)<0 || STKVAL(0)>=(1<<pfx))
	  errormsg("value is out of range %d bits unsigned", (int)pfx);
	STKVAL(0) &= (1<<pfx)-1;
	pfx = 0;
	break;
      case OP_CHECKS:
	if(STKVAL(0)<-(1<<(pfx-1)) || STKVAL(0)>=(1<<(pfx-1)))
	  errormsg("value is out of range %d bits signed", (int)pfx);
	STKVAL(0) &= (1<<pfx)-1;
	pfx = 0;
	break;
      case OP_CHECKW:
	if(STKVAL(0)<1 || STKVAL(0)>(1<<pfx))
	  errormsg("value is out of range %d bits wraparound", (int)pfx);
	STKVAL(0) &= (1<<pfx)-1;
	pfx = 0;
	break;
      case OP_CHECKX:
	if(STKVAL(0)<-(1<<pfx) || STKVAL(0)>=(1<<pfx))
	  errormsg("value is out of range %d bits", (int)pfx);
	STKVAL(0) &= (1<<pfx)-1;
	pfx = 0;
	break;
	/*
      case OP_STPREFIX(1): EMIT1(pfx); pfx=0; break;
      case OP_STPREFIX(2): EMIT2(pfx); pfx=0; break;
      case OP_STPREFIX(3): EMIT3(pfx); pfx=0; break;
      case OP_STPREFIX(4): EMIT4(pfx); pfx=0; break;
      case OP_STPREFIX(5): EMIT5(pfx); pfx=0; break;
      case OP_STPREFIX(6): EMIT6(pfx); pfx=0; break;
      case OP_STPREFIX(7): EMIT7(pfx); pfx=0; break;
	*/
      case OP_STINLINE(8):  EMITI(1,p); p++; break;
      case OP_STINLINE(16): EMITI(2,p); p+=2; break;
      case OP_STINLINE(24): EMITI(3,p); p+=3; break;
      case OP_STINLINE(32): EMITI(4,p); p+=4; break;
      case OP_STINLINE(40): EMITI(5,p); p+=5; break;
      case OP_STINLINE(48): EMITI(6,p); p+=6; break;
      case OP_STINLINE(56): EMITI(7,p); p+=7; break;
      case OP_STORE(8): EMIT8(POP); break;
      case OP_STORE(16): EMIT16(POP); break;
      case OP_STORE(24): EMIT24(POP); break;
      case OP_STORE(32): EMIT32(POP); break;
      case OP_STORE(40): EMIT40(POP); break;
      case OP_STORE(48): EMIT48(POP); break;
      case OP_STORE(56): EMIT56(POP); break;
      case OP_STOREPB: EMITN(pfx,POP); pfx=0; break;
      case OP_STINLINEPB: EMITI(pfx,p); p+=(pfx+7)>>3; pfx=0; break;
      case OP_ORG: { numtype a = POP; while(*current_loc < a) EMIT8(0); break;}
      case OP_CNOP: { numtype b = findmsb(pfx); pfx &= (b-1);
                      while(((*current_loc)&(b-1)) != pfx) EMIT8(0); pfx=0;
                      break; }
      case OP_LINENO: current_lineno += pfx+1; pfx=0; break;
      case OP_FILENAME: if(pfx) { filestack_push(pfx-1); current_lineno=0;
				  pfx=0; } else filestack_pop(); break;
	
      case OP_LOADL:
	{
	  struct symbol *s = locate_sym(pfx);
	  PUSH((s->value.num==0xdeadbeef? *current_loc : s->value.num));
	  pfx=0;
	  break;
	}
      case OP_STOREL:
	{
	  struct symbol *s = locate_sym(pfx);
	  numtype n = POP;
	  if(n != s->value.num) {
	    s->value.num = n;
	    if(changed == NULL)
	      changed = s;
	  }
	  pfx=0;
	  break;
	}

      default:
	fprintf(stderr, "Internal error: smach:smach_execute(%02x)\n", o);
	exit(3);
      } else {
	numtype n = pfx<<5|(o&0x1f);
	PUSH((o&0x20)?~n:n);
	pfx = 0;
      }
    } else {
      pfx <<= 7;
      pfx |= o;
      if(pfx == 0) {
	fprintf(stderr, "Internal error: smach:pfx == 0\n");
	exit(3);
      }
    }
  }
}

struct symbol *smach_run()
{
  int p;
  numtype pfx = 0;
  changed = NULL;
  current_lineno = 0;
  reset_sections();
  for(p=0; p<regd_blks; p++)
    pfx = smach_execute(code_blk[p], pfx);
  return changed;
}

void smach_init()
{
  SMVs = NULL;
  numSMVs = maxSMVs = 0;
  emit_p = emit_brk = NULL;
  code_blk = NULL;
  regd_blks = allocated_blks = 0;
  saved_lineno = 0;
}

void smach_end()
{
  if(code_blk != NULL) {
    int i;
    for(i=0; i<regd_blks; i++)
      free(code_blk[i]);
    free(code_blk);
  }
  if(SMVs) free(SMVs);
}
