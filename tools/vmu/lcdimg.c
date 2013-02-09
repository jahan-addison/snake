#ifdef WIN32
#include <windows.h>
#include <io.h>
#define HAVE_FCNTL_H
#define HAVE_ERRNO_H
#endif

#ifdef __DJGPP__
#include <io.h>
#include <sys/timeb.h>
#define HAVE_FCNTL_H
#define HAVE_ERRNO_H
#define HAVE_SYS_TIME_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_DEVICES_TIMER_H
#include <devices/timer.h>
#undef tv_secs
#undef tv_micro
#define tv_sec tv_secs
#define tv_usec tv_micro
#endif

#include "prototypes.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif


static int lcdi_width, lcdi_height, lcdi_nframes;
static long *lcdi_frametime;
static unsigned char **lcdi_framedata;

static void display_frame(int f)
{
  unsigned char *p = lcdi_framedata[f];
  int x, y, w = lcdi_width, h = lcdi_height;
  if(w>48)
    w = 48;
  if(h>32)
    h = 32;
  for(y=0; y<h; y++) {
    unsigned char *p2 = p;
    for(x=0; x<w; x++)
      vmputpixel(x, y, !!*p2++);
    p += lcdi_width;
  }
  redrawlcd();
}

static void free_framedata()
{
  int i;
  for(i=0; i<lcdi_nframes; i++)
    if(lcdi_framedata[i] != NULL)
      free(lcdi_framedata[i]);
  free(lcdi_framedata);
}

static int myread(int fd, unsigned char *data, int len)
{
  int r, rtot=0;
  while(len>0) {
    r = read(fd, data, len);
    if(r == 0 || r >= len)
      return r+rtot;
    if(r < 0) {
      if(errno == EAGAIN)
	sleep(1);
      else if(errno != EINTR)
	return -1;
    } else {
      data += r;
      len -= r;
      rtot += r;
    }
  }
  return rtot;
}

int initimg(int fd)
{
  unsigned char hdr[16], tm[4];
  int i, fsz;
  if(myread(fd, hdr, 16)<16)
    return 0;
  if(strncmp(hdr, "LCDi", 4))
    return 0;
  lcdi_width = hdr[6]+(hdr[7]<<8);
  lcdi_height = hdr[8]+(hdr[9]<<8);
  lcdi_nframes = hdr[14]+(hdr[15]<<8);
  fsz = lcdi_width * lcdi_height;
  if((lcdi_frametime = calloc(lcdi_nframes, sizeof(long)))==NULL)
    return 0;
  for(i=0; i<lcdi_nframes; i++) {
    if(myread(fd, tm, 4)<4) {
      free(lcdi_frametime);
      return 0;
    }
    lcdi_frametime[i] = tm[0]+(tm[1]<<8)+(tm[2]<<16)+(tm[3]<<24);
  }
  if((lcdi_framedata = calloc(lcdi_nframes, sizeof(unsigned char *)))==NULL) {
    free(lcdi_frametime);
    return 0;
  }
  for(i=0; i<lcdi_nframes; i++)
    lcdi_framedata[i] = NULL;
  for(i=0; i<lcdi_nframes; i++)
    if((lcdi_framedata[i] = malloc(fsz))==NULL ||
       myread(fd, lcdi_framedata[i], fsz)<fsz) {
      free_framedata();
      free(lcdi_frametime);
      return 0;
    }
  return 1;
}

void freeimg()
{
  free_framedata();
  free(lcdi_frametime);  
}

int loadimg(char *filename)
{
  int fd;

  if((fd = open(filename, O_RDONLY|O_BINARY))>=0) {
    if(!initimg(fd)) {
      close(fd);
      error_msg("%s: can't decode image", filename);
      return 0;
    }
    close(fd);
    return 1;
  } else {
    perror(filename);
    return 0;
  }
}

int do_lcdimg(char *filename)
{
  extern unsigned char sfr[0x100];

  if(!loadimg(filename))
    return 0;
  sfr[0x4c] = 0xff;
  if(lcdi_nframes>1) {
    struct timeval tnow, tnext, tdel;
    int fcntr=-1;
    GETTIMEOFDAY(&tnext);
    for(;;) {
      GETTIMEOFDAY(&tnow);
      if(tnow.tv_sec > tnext.tv_sec ||
	 (tnow.tv_sec == tnext.tv_sec && tnow.tv_usec >= tnext.tv_usec)) {
	if((++fcntr>=lcdi_nframes))
	  fcntr=0;
	display_frame(fcntr);
	tnext.tv_sec += lcdi_frametime[fcntr]/1000;
	if((tnext.tv_usec += (lcdi_frametime[fcntr]%1000)*1000)>=1000000) {
	  tnext.tv_sec++;
	  tnext.tv_usec -= 1000000;
	}
      } else {
	tdel.tv_sec = tnext.tv_sec - tnow.tv_sec;
	if((tdel.tv_usec = tnext.tv_usec - tnow.tv_usec)<0) {
	  tdel.tv_sec--;
	  tdel.tv_usec += 1000000;
	}
	waitforevents(&tdel);
      }
      checkevents();
      if((sfr[0x4c]&0x70)!=0x70)
	break;
    }
  } else if(lcdi_nframes==1) {
    display_frame(0);
    for(;;) {
      waitforevents(NULL);
      checkevents();
      if((sfr[0x4c]&0x70)!=0x70)
	break;
    }
  }
  freeimg();
  return 1;
}
