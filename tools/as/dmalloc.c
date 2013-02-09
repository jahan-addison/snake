#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_MALLOC

extern char *strdup(const char *);

int verbose_debug_malloc = 0;
int verbose_debug_exit = 1;

static struct memhdr {
  struct memhdr *next;
  void *data;
  size_t size;
  const char *filename;
  int lineno;
} *memhdrs=NULL;

static struct memhdr *find_memhdr(void *p)
{
  struct memhdr *mh;
  for(mh=memhdrs; mh; mh=mh->next)
    if(mh->data==p)
      return mh;
  return NULL;
}

static void make_memhdr(void *p, int s, const char *fn, int ln)
{
  struct memhdr *mh=malloc(sizeof(struct memhdr));
  if(!mh) {
    fprintf(stderr, "Fatal: coudln't allocate memhdr structure\n");
    exit(17);
  }
  mh->next=memhdrs;
  mh->data=p;
  mh->size=s;
  mh->filename=fn;
  mh->lineno=ln;
  memhdrs=mh;
}

static void remove_memhdr(struct memhdr *mh)
{
  struct memhdr *mh2;
  if(mh==memhdrs)
    memhdrs=mh->next;
  else for(mh2=memhdrs; mh2->next; mh2=mh2->next)
    if(mh2->next==mh) {
      mh2->next=mh->next;
      break;
    }
  free(mh);
}

void *debug_malloc(size_t s, const char *fn, int ln)
{
  void *m=malloc(s);

  if(m)
    make_memhdr(m, s, fn, ln);
  else
    fprintf(stderr, "Notice: malloc(%d) (%s line %d) failed\n", s, fn, ln);
  if(verbose_debug_malloc)
    fprintf(stderr, "malloc(%d) => %p  (%s line %d)\n", s, m, fn, ln);
  return m;
}

void *debug_calloc(size_t a, size_t b, const char *fn, int ln)
{
  void *m=calloc(a, b);

  if(m)
    make_memhdr(m, a*b, fn, ln);
  else
    fprintf(stderr, "Notice: calloc(%d,%d) (%s line %d) failed\n",
	    a, b, fn, ln);
  if(verbose_debug_malloc)
    fprintf(stderr, "calloc(%d,%d) => %p  (%s line %d)\n", a, b, m, fn, ln);
  return m;
}

void *debug_realloc(void *p, size_t s, const char *fn, int ln)
{
  struct memhdr *mh=find_memhdr(p);
  void *m;
  if(!mh) {
    fprintf(stderr, "Fatal: realloc(%p,%d) (%s line %d): no such allocation\n",
	    p, s, fn, ln);
    exit(17);
  }
  m=realloc(p, s);
  if(m) {
    mh->data=m;
    mh->size=s;
  } else {
    fprintf(stderr, "Notice: realloc(%p,%d) (%s line %d) failed\n",
	    p, s, fn, ln);
    remove_memhdr(mh);
  }
  if(verbose_debug_malloc)
    fprintf(stderr, "realloc(%p,%d) => %p  (%s line %d)\n", p, s, m, fn, ln);
  return m;
}

void debug_free(void *p, const char *fn, int ln)
{
  struct memhdr *mh=find_memhdr(p);
  if(!mh) {
    fprintf(stderr, "Fatal: free(%p) (%s line %d): no such allocation\n",
	    p, fn, ln);
    exit(17);
  }
  free(mh->data);
  remove_memhdr(mh);  
  if(verbose_debug_malloc)
    fprintf(stderr, "free(%p)  (%s line %d)\n", p, fn, ln);
}

char *debug_strdup(const char *s, const char *fn, int ln)
{
  char *m=strdup(s);

  if(m)
    make_memhdr(m, strlen(s)+1, fn, ln);
  else
    fprintf(stderr, "Notice: strdup(\"%s\") (%s line %d) failed\n", s, fn, ln);
  if(verbose_debug_malloc)
    fprintf(stderr, "strdup(\"%s\") => %p  (%s line %d)\n", s, m, fn, ln);
  return m;
}

static void cleanup_memhdrs()
{
  while(memhdrs) {
    if(verbose_debug_exit)
      fprintf(stderr, "Notice: Block at %p (%d bytes) not freed\n\t(Allocated by %s line %d)\n", memhdrs->data, memhdrs->size, memhdrs->filename, memhdrs->lineno);
    free(memhdrs->data);
    remove_memhdr(memhdrs);
  }
}

int main(int argc, char *argv[])
{
  extern int dbm_main(int, char **);
  atexit(cleanup_memhdrs);
  return dbm_main(argc, argv);
}

#endif
