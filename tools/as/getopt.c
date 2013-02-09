#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int optind=1;
char *optarg=NULL;
int optopt;
int opterr=1;
static int optskip=0;


int getopt(int argc, char * const * argv, const char *optstring)
{
  char *opt;

  if(optskip) {
    optarg="";
    optskip=0;
  }
  if(!optarg) {
    --optind;
    optarg="";
  }
  while(!*optarg)
    if(++optind>=argc)
      return EOF;
    else {
      optarg=argv[optind];
      if(*optarg++!='-' || !*optarg)
	return EOF;
      if(optarg[0]=='-' && !optarg[1]) {
	optind++;
	return EOF;
      }
    }
  if((opt=strchr(optstring, optopt=*optarg++)) && optopt!=':') {
    if(opt[1]==':') {
      optskip=1;
      if(!*optarg)
	if(++optind<argc)
	  optarg=argv[optind];
        else {
	  if(opterr)
	    fprintf(stderr, "%s: option '-%c' requires an argument\n", argv[0], optopt);
	  return '?';
	}
    }
    return optopt;
  } else {
    if(opterr)
      fprintf(stderr, "%s: invalid option -- %c\n", argv[0], optopt);
    return '?';
  }
}
