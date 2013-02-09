#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(int argc, char *argv[])
{
  register char *not;
  register int i, j, l, m, n=2;

  if(argc!=2) { fprintf(stderr, "usage: sieve <number>\n"); exit(1); }
  if((m=atoi(argv[1]))<3) { fprintf(stderr, "Need higher cieling\n"); exit(1); }
  l=(m+1)>>1;
  if(!(not=malloc(l))) { fprintf(stderr, "Not enough memory\n"); exit(1); }
  memset(not, 0, l);
  for(i=1; i<l; i++) if(!not[i]) {
    n=(i<<1)+1;
    for(j=i+n; j<l; j+=n) not[j]=1;
  }
  free(not);
  printf("%d\n", n);
}
