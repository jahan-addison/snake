#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taz.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

struct mempool macrotextpool;

static struct macarg {
  int offs, len;
} *macargs;
static int nummacargs=0, maxmacargs=0;
static char *macargbuf;
static int macargbufuse=0, macargbufmax=0;

struct symbol *findmacro(const char *name)
{
  struct symbol *s=lookup_symbol(name);
  if(s && s->type!=SYMB_MACRO) s=NULL;
  return s;
}

void macro_storearg(int n, const char *arg)
{
  int l=strlen(arg);
  if(!n) { nummacargs=0; macargbufuse=0; }
  if(nummacargs>=maxmacargs)
    if(macargs)
      macargs=realloc(macargs, sizeof(struct macarg)*(maxmacargs<<=1));
    else
      macargs=malloc(sizeof(struct macarg)*(maxmacargs=10));
  if(!macargs) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  macargs[nummacargs].offs=macargbufuse;
  macargs[nummacargs].len=l;
  nummacargs++;
  if(macargbufuse+l>=macargbufmax)
    if(macargbuf)
      macargbuf=realloc(macargbuf, macargbufmax=min(macargbufmax<<1, macargbufuse+l+1));
    else
      macargbuf=malloc(macargbufmax=min(256, l+1));
  if(!macargbuf) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  strcpy(macargbuf+macargbufuse, arg);
  macargbufuse+=l+1;
}

void invoke_macro(struct symbol *m, int n)
{
  extern struct yy_buffer_state *yy_scan_buffer(char *, unsigned int);
  int l=2, batlen;
  char *p, *p2, *b, bat[20];
  static int invokation=0;

  sprintf(bat, "_%04d", invokation++);
  batlen=strlen(bat);

  for(p=m->value.macro; *p;)
    if(*p=='\\')
      if(p[1]>='0' && p[1]<='9') {
	long x=strtol(p+1, &p, 10);
	if(x>0 && x<=n)
	  l+=macargs[x-1].len;
      } else if(p[1]=='@') {
	l+=batlen;
	p+=2;
      } else {
	l++;
	p++;
	if(*p) {
	  l++;
	  p++;
	}
      }
    else {
      l++;
      p++;
    }

  p2=b=malloc(l);
  if(!b) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  for(p=m->value.macro; *p;)
    if(*p=='\\')
      if(p[1]>='0' && p[1]<='9') {
	long x=strtol(p+1, &p, 10);
	if(x>0 && x<=n) {
	  strcpy(p2, macargbuf+macargs[x-1].offs);
	  p2+=macargs[x-1].len;
	}
      } else if(p[1]=='@') {
	strcpy(p2, bat);
	p2+=batlen;	
	p+=2;
      } else {
	*p2++=*p++;
	if(*p) *p2++=*p++;
      }
    else
      *p2++=*p++;
  *p2++='\0';
  *p2++='\0';

  process_buffer(m->defined_file, m->defined_line,
		 yy_scan_buffer(b, l), NULL, b);
}

void macro_init()
{
  initpool(&macrotextpool, 1, 8000);
  macargs=NULL;
  macargbuf=NULL;
  nummacargs=maxmacargs=macargbufuse=macargbufmax=0;
}

void macro_end()
{
  emptypool(&macrotextpool);
  if(macargs)
    free(macargs);
  if(macargbuf)
    free(macargbuf);
}
