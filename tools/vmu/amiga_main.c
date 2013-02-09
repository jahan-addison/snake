#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include "version.h"
#include "prototypes.h"

extern struct Window *openmainwin(struct Screen *dpy);

extern int pixdbl;

extern int optind;
extern char *optarg;
extern int getopt(int argc, char * argv[], char *optstring);

int main(int argc, char *argv[])
{
  struct Screen *dpy;
  int c;
  extern int optind;
  char *bios = NULL;

  while((c=getopt(argc, argv, "hV2b:"))!=EOF)
    switch(c) {
     case 'h':
       fprintf(stderr, "Usage: %s [-h] [-V] [-2] [-b bios] game.vms ...\n",
	       argv[0]);
       break;
     case 'V':
       fprintf(stderr, "softvms " VERSION " by Marcus Comstedt <marcus@idonex.se>\n");
       break;
     case '2':
       pixdbl = 1;
       break;
     case 'b':
       bios = optarg;
       break;
    }
  dpy = LockPubScreen(NULL);
  if(dpy == NULL) {
    fprintf(stderr, "Failed to lock default public screen\n");
    return 1;
  }
  if(!openmainwin(dpy))
    return 1;
  if(argc>optind) {
    int fd = open(argv[optind], O_RDONLY);
    int r;
    char hdr[4];
    if(fd<0 || (r=read(fd, hdr, 4)<4)) {
      perror(argv[optind]);
      if(fd>=0)
	close(fd);
      return 1;
    }
    if(hdr[0]=='L' && hdr[1]=='C' && hdr[2]=='D' && hdr[3]=='i')
      do_lcdimg(argv[optind]);
    else
      do_vmsgame(argv[optind], bios);
  } else if(bios) {
    do_vmsgame(NULL, bios);
  } else
    for(;;) {
      waitforevents(NULL);
      checkevents();
    }
  return 0;
}
