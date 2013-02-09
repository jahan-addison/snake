#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "taz.h"
#include "hashsize.h"

static struct symbol *hashtable[HASHSIZE];

struct mempool symbolpool, symbolnamepool;
struct symbol *currloc_sym;
number *current_loc = NULL;
static int localsyms;

static int hash(const char *s)
{
  unsigned int h=0;
  while(*s) h=h*17+((*s++)&0xdf);
  return h%HASHSIZE;
}

struct symbol *lookup_symbol(const char *name)
{
  struct symbol *s=hashtable[hash(name)];
  while(s)
    if(!strcasecmp(s->name, name))
      return s;
    else
      s=s->chain;
  return NULL;
}

struct symbol *create_symbol(char *name, int type)
{
  struct symbol *s;
  if((s=lookup_symbol(name))) {
    if((type==SYMB_SET && (s->type==SYMB_SET || s->type==SYMB_UNDSET)) ||
       (s->type==SYMB_UNDEF && (type==SYMB_EQU || type==SYMB_RELOC))) {
      if(s->type==SYMB_UNDEF) {
	s->type=type;
	s->defined_file=current_filename;
	s->defined_line=current_lineno;
      }
      poolfreestr(&symbolnamepool, name);
      return s;
    }
    errormsg("label %s already defined in %s line %d",
	     name, s->defined_file, s->defined_line);
    return NULL;
  } else {
    int h=hash(name);
    s=poolalloc(&symbolpool);
    s->name=name;
    s->type=type;
    s->defined_file=current_filename;
    s->defined_line=current_lineno;
    s->chain=hashtable[h];
    hashtable[h]=s;
    if(name[0]=='.')
      localsyms++;
    return s;
  }
}

int symbol_check()
{
  int h, r=0;
  for(h=0; h<HASHSIZE; h++) {
    struct symbol *s=hashtable[h];
    while(s) {
      if(s->type == SYMB_UNDEF) {
	fprintf(stderr, "%s line %d: undefined label %s\n",
		s->defined_file, s->defined_line, s->name);
	numerrors++;
	r = 1;
      }
      s=s->chain;
    }
  }
  return r;
}

void flushlocalsyms()
{
  int h;

  if(!localsyms)
    return;

  for(h=0; h<HASHSIZE; h++) {
    struct symbol *s, **backlink = &hashtable[h];
    while((s=*backlink)!=NULL) {
      if(s->name[0]=='.') {
	if(s->type == SYMB_UNDEF) {
	  fprintf(stderr, "%s line %d: undefined label %s\n",
		  s->defined_file, s->defined_line, s->name);
	  numerrors++;
	}
	*backlink = s->chain;
      } else
	backlink = &s->chain;
    }
  }

  localsyms = 0;
}


void symbol_init()
{
  struct symbol *s;
  int i;

  localsyms = 0;
  initpool(&symbolpool, sizeof(struct symbol), 1000);
  initpool(&symbolnamepool, 1, 8000);
  for(i=0; i<HASHSIZE; i++)
    hashtable[i]=NULL;
  if((s=create_symbol(poolstring(&symbolnamepool, "__TAZ__"), SYMB_EQU))) {
    s->value.num=1;
    s->defined_file="<Init>";
    s->defined_line=0;
  }
  if((s=create_symbol(poolstring(&symbolnamepool, "*"), SYMB_UNDSET))) {
    s->value.num=0;
    s->defined_file="<Init>";
    s->defined_line=0;
    currloc_sym = s;
    current_loc = &s->value.num;
  } else {
    fprintf(stderr, "Fatal: can't create symbol `*'\n");
    exit(2);
  }
}

void symbol_end()
{
  emptypool(&symbolpool);
  emptypool(&symbolnamepool);
}
