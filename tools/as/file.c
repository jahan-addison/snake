#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "taz.h"
#include "smach.h"

extern void yyrestart(FILE *);
extern void yy_switch_to_buffer(struct yy_buffer_state *);
extern struct yy_buffer_state *yy_create_buffer(FILE *, int);
extern void yy_delete_buffer(struct yy_buffer_state *);
extern FILE *yyin;

int numerrors, numlines;

static struct mempool filenamepool;

static struct file_info {
  const char *filename;
  int lineno;
  FILE *file;
  struct yy_buffer_state *buffer;
  void *block;
} *filestack;
static struct file_brief {
  const char *filename;
  int is_macro;
} *allfiles;
static int filestackdepth=0, numfileinfos=0, numids=0, numfilebriefs=0;
static int numincdirs=0, maxincdirs=0, maxincdirlen=0;

int current_lineno;
const char *current_filename, **incdir;

static int make_id(const char *filename, int is_macro)
{
  int i;
  for(i=0; i<numids; i++)
    if(allfiles[i].filename == filename &&
       allfiles[i].is_macro == is_macro)
      return i;
  if(numids >= numfilebriefs)
    if(allfiles)
      allfiles=realloc(allfiles,
		       sizeof(struct file_brief)*(numfilebriefs<<=1));
    else
      allfiles=malloc(sizeof(struct file_brief)*(numfilebriefs=32));
  allfiles[numids].filename = filename;
  allfiles[numids].is_macro = is_macro;
  return numids++;
}

void filestack_push(int id)
{
  struct file_brief *f = &allfiles[id];
  if(filestackdepth>=numfileinfos) {
    fprintf(stderr, "Internal error: Filestack grew in pass N???\n");
    exit(2);
  }
  if(filestackdepth)
    filestack[filestackdepth-1].lineno = current_lineno;
  current_filename = filestack[filestackdepth].filename = f->filename;
  filestack[filestackdepth++].file = (f->is_macro? NULL : stdout);
}

void filestack_pop()
{
  if(--filestackdepth) {
    current_lineno = filestack[filestackdepth-1].lineno;
    current_filename = filestack[filestackdepth-1].filename;
  }
}

void process_buffer(const char *filename, int lineno,
		    struct yy_buffer_state *buf, FILE *f, void *block)
{
  extern int maxrecurse;
  if(filestackdepth>=maxrecurse) {
    fprintf(stderr, "Fatal: Too many levels of include/macro!\n");
    exit(2);
  }
  if(!buf) {
    fprintf(stderr, "Fatal: Internal error - no yy_buffer_state\n");
    exit(2);
  }
  if(filestackdepth>=numfileinfos)
    if(filestack)
      filestack=realloc(filestack,
			sizeof(struct file_info)*(numfileinfos<<=1));
    else
      filestack=malloc(sizeof(struct file_info)*(numfileinfos=10));
  if(!filestack) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  if(filestackdepth)
    filestack[filestackdepth-1].lineno = current_lineno;
  current_filename = filestack[filestackdepth].filename = filename;
  yy_switch_to_buffer(filestack[filestackdepth].buffer = buf);
  filestack[filestackdepth].block = block;
  if((filestack[filestackdepth++].file = f))
    yyrestart(f);
  smach_pushfile(make_id(filename, f == NULL));
  current_lineno = lineno;
}

int process_file(const char *filename)
{
  FILE *f;

  if((f=fopen(filename, "r"))) {
    process_buffer(poolstring(&filenamepool, filename), 0,
		   yy_create_buffer(f, 16384), f, NULL);
    ungetc('\n', f);
    return 1;
  } else return 0;
}

int process_include(const char *filename)
{
  int i;
  if(filename[0]=='/')
    return process_file(filename);
  else {
    char *buf=malloc(strlen(filename)+maxincdirlen+2);
    if(!buf) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }
    for(i=0; i<numincdirs; i++) {
      if(*incdir[i])
	sprintf(buf, "%s/%s", incdir[i], filename);
      else
	strcpy(buf, filename);
      if(process_file(buf)) {
	free(buf);
	return 1;
      }
    }
    free(buf);
    return 0;
  }
}

void add_incdir(const char *name)
{
  int l;

  if(numincdirs<=maxincdirs)
    if(incdir)
      incdir=realloc(incdir, sizeof(const char *)*(maxincdirs<<=1));
    else
      incdir=malloc(sizeof(const char *)*(maxincdirs=10));  
  if(!incdir) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  incdir[numincdirs++]=poolstring(&filenamepool, name);
  if((l=strlen(name))>maxincdirlen)
    maxincdirlen=l;
}

void errormsg(const char *format, ...)
{
  va_list va;
  int i;
  fprintf(stderr, "%s line %d: ", current_filename, current_lineno);
  va_start(va, format);
  vfprintf(stderr, format, va);
  va_end(va);
  putc('\n', stderr);
  for(i=filestackdepth-2; i>=0; --i)
    if(filestack[i+1].file)
      fprintf(stderr, " - in file included from %s line %d\n",
	      filestack[i].filename, filestack[i].lineno);
    else
      fprintf(stderr, " - in macro invoked from %s line %d\n",
	      filestack[i].filename, filestack[i].lineno);
  numerrors++;
}

int file_yywrap()
{
  yy_delete_buffer(filestack[filestackdepth-1].buffer);
  if(filestack[filestackdepth-1].file)
    fclose(filestack[filestackdepth-1].file);
  if(filestack[filestackdepth-1].block)
    free(filestack[filestackdepth-1].block);
  if(--filestackdepth) {
    yy_switch_to_buffer(filestack[filestackdepth-1].buffer);
    current_lineno = filestack[filestackdepth-1].lineno;
    current_filename = filestack[filestackdepth-1].filename;
    yyin = filestack[filestackdepth-1].file;
    smach_popfile();
    return 0;
  } else {
    smach_popfile();
    return 1;
  }
}

int abort_macro()
{
  if(filestackdepth<1 || filestack[filestackdepth-1].file!=NULL)
    return 0;
  file_yywrap();
  return 1;
}

void file_init()
{
  initpool(&filenamepool, 1, 1000);
  filestack=NULL;
  allfiles=NULL;
  incdir=NULL;
  filestackdepth=numfileinfos=numincdirs=maxincdirs=maxincdirlen=0;
}

void file_end()
{
  emptypool(&filenamepool);
  if(filestack!=NULL) free(filestack);
  if(allfiles!=NULL) free(allfiles);
  if(incdir!=NULL) free(incdir);
}
