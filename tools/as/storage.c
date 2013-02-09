#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "taz.h"

struct memhdr {
  struct memhdr *next;
  int numfree;
};

void initpool(struct mempool *mp, int allocsize, int clustersize)
{
  mp->block1=NULL;
  mp->allocsize=allocsize;
  mp->clustersize=clustersize;
  mp->headersize=(sizeof(struct memhdr)+allocsize-1)/allocsize;
  mp->totfree=0;
}

void emptypool(struct mempool *mp)
{
  struct memhdr *m, *n;
  for(n=mp->block1; (m=n); ) {
    n=m->next;
    free(m);
  }
  mp->block1=NULL;
}

void *poolallocn(struct mempool *mp, int n)
{
  struct memhdr *mh=mp->block1;
  int tf=mp->totfree;
  while(mh && mh->numfree<n && (tf-=mh->numfree)>=n) mh=mh->next;
  if((!mh)||mh->numfree<n) {
    mh=calloc(mp->clustersize+mp->headersize, mp->allocsize);
    if(!mh) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }
    mh->next=mp->block1;
    mp->totfree+=(mh->numfree=mp->clustersize);
    mp->block1=mh;
  }
  mp->totfree-=n;
  mh->numfree-=n;
  return ((char *)mh)+mp->allocsize*(mh->numfree+mp->headersize);
}

void poolfreen(struct mempool *mp, void *p, int n)
{
  struct memhdr *mh;

  for(mh=mp->block1; mh; mh=mh->next)
    if(p==(((char *)mh)+mp->allocsize*(mh->numfree+mp->headersize))) {
      mh->numfree+=n;
      return;
    }
}

char *poolstring(struct mempool *mp, const char *str)
{
  char *dest=poolallocn(mp, strlen(str)+1);
  strcpy(dest, str);
  return dest;
}
