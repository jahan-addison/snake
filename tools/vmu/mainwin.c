#include <X11/Xlib.h>
#include <X11/keysym.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "prototypes.h"
#ifdef USE_SDL_AUDIO
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
#endif

#include "vmu.xpm"
#include "vmu.5.xpm"

static Window mainwin;
static XImage *mainimg;
static GC maingc;
static Display *maindpy;

int pixdbl = 0;
int sound_freq = -1;

void error_msg(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);
}

#ifdef USE_SDL_AUDIO
static void sound_callback(void *userdata, Uint8 *stream, int len)
{
  if(sound_freq <= 0)
    memset(stream, 0, len);
  else {
    int i;
    static char v = 0x7f;
    static int f = 0;
    for(i=0; i<len; i++) {
      f += sound_freq;
      while(f >= 32768) {
	v ^= 0xff;
	f -= 32768;
      }
      *stream++ = v;
    }
  }
}

void init_sound()
{
  static SDL_AudioSpec desired = {
    32768,
    AUDIO_S8,
    1,
    0, 256, 0, 0,
    sound_callback,
    NULL
  };

  if(SDL_Init(SDL_INIT_AUDIO)<0)
    fprintf(stderr, "Failed to SDL_Init, sound disabled.\n");
  else if(SDL_OpenAudio(&desired, NULL)<0)
    fprintf(stderr, "Failed to SDL_OpenAudio, sound disabled.\n");
  else
    SDL_PauseAudio(0);
}
#endif

static Status myXAllocNamedColor(Display *dpy, Colormap cmap, char *color_name,
				 XColor *screen_def, XColor *exact_def)
{
  Status s;

  if((s=XAllocNamedColor(dpy, cmap, color_name, screen_def, exact_def)))
    return s;
  if((s=XLookupColor(dpy, cmap, color_name, screen_def, exact_def))||
     (s=XParseColor(dpy, cmap, color_name, exact_def))) {
    *screen_def = *exact_def;
    screen_def->pixel = 0;
    screen_def->flags = DoRed|DoGreen|DoBlue;
    s=XAllocColor(dpy, cmap, screen_def);
  }
  return s;
}

static Pixmap mkbgpixmap(Display *dpy, Window win, char **vmu1_xpm, int *wret, int *hret)
{
  int i, w, h, nc, x, y, bpl, bpad;
  unsigned long pixel[1<<CHAR_BIT];
  XColor scr, exa;
  XImage *img;
  char *data;
  Pixmap pm;
  GC gc;
  XWindowAttributes attr;
  XPixmapFormatValues *fmts;

  XGetWindowAttributes(dpy, win, &attr);
  if((fmts = XListPixmapFormats(dpy, &nc))==NULL) {
    fprintf(stderr, "Out of memory\n");
    return None;
  }
  for(x=-1, i=0; i<nc; i++)
    if(fmts[i].depth == attr.depth) {
      x=i;
      bpad = fmts[i].scanline_pad;
      bpl = fmts[i].bits_per_pixel;
    }
  XFree(fmts);
  if(x<0) {
    fprintf(stderr, "Can't find a pixmap format for depth %d!\n", attr.depth);
    return None;
  }
  sscanf(vmu1_xpm[0], "%d %d %d", &w, &h, &nc);
  for(i=1; i<=nc; i++)
    if(myXAllocNamedColor(dpy, attr.colormap, vmu1_xpm[i]+4, &scr, &exa))
      pixel[*(unsigned char *)vmu1_xpm[i]] = scr.pixel;
    else {
      fprintf(stderr, "Failed to allocate color %s\n", vmu1_xpm[i]+4);
      return None;
    }
  bpl = (((bpl*w+bpad-1)&~(bpad-1))+7)>>3;
  if((data = malloc(bpl*h)) == NULL) {
    fprintf(stderr, "Out of memory\n");
    return None;    
  }
  if((img = XCreateImage(dpy, attr.visual, attr.depth, ZPixmap, 0, data,
			 w, h, bpad, bpl))==NULL) {
      fprintf(stderr, "Failed to allocate XImage\n");
      free(data);
      return None;
  }
  for(y=0; y<h; y++) {
    unsigned char *p = (unsigned char *)vmu1_xpm[i++];
    for(x=0; x<w; x++)
      XPutPixel(img, x, y, pixel[*p++]);
  }
  pm = XCreatePixmap(dpy, win, w, h, attr.depth);
  gc = XCreateGC(dpy, pm, 0, NULL);
  XPutImage(dpy, pm, gc, img, 0, 0, 0, 0, w, h);
  XFreeGC(dpy, gc);
  XDestroyImage(img);
  *wret = w;
  *hret = h;
  return pm;
}

static XImage *mkmainimg(Display *dpy, Window win, int w, int h)
{
  XWindowAttributes attr;
  XPixmapFormatValues *fmts;
  int x, y, i, nc, bpad, bpl;
  char *data;
  XImage *img;

  if((fmts = XListPixmapFormats(dpy, &nc))==NULL) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  for(x=-1, i=0; i<nc; i++)
    if(fmts[i].depth == 1) {
      x=i;
      bpad = fmts[i].scanline_pad;
      bpl = fmts[i].bits_per_pixel;
    }
  XFree(fmts);
  if(x<0) {
    fprintf(stderr, "Can't find a bitmap format for depth 1!\n");
    return NULL;
  }
  bpl = (((bpl*w+bpad-1)&~(bpad-1))+7)>>3;
  if((data = malloc(bpl*h)) == NULL) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }
  XGetWindowAttributes(dpy, win, &attr);
  if((img = XCreateImage(dpy, attr.visual, 1, XYBitmap, 0, data,
			 w, h, bpad, bpl))==NULL) {
    fprintf(stderr, "Failed to allocate XImage\n");
    free(data);
    return NULL;
  }
  for(y=0; y<h; y++)
    for(x=0; x<w; x++)
      XPutPixel(img, x, y, 0);
  return img;
}

Window openmainwin(Display *dpy)
{
  Window root;
  XWindowAttributes attr;
  XSetWindowAttributes swa;
  XGCValues gcv;
  Pixmap bgpm;
  XColor exa, scr1, scr2;
  int w, h;

  maindpy = dpy;
  root = DefaultRootWindow(dpy);
  XGetWindowAttributes(dpy, root, &attr);
  if((bgpm = mkbgpixmap(dpy, root, (pixdbl? vmu_large:vmu_small), &w, &h))==None)
    return None;
  swa.background_pixmap = bgpm;
  swa.backing_store = NotUseful;
  swa.colormap = attr.colormap;
  swa.bit_gravity = NorthWestGravity;
  mainwin = XCreateWindow(dpy, root, 0, 0, w, h, 0, attr.depth,
			  InputOutput, attr.visual,
			  CWBackPixmap|CWBackingStore|CWColormap|CWBitGravity,
			  &swa);
  if((mainimg = mkmainimg(dpy, mainwin, (pixdbl? 96:48), (pixdbl? 80:40)))==NULL) {
    XDestroyWindow(dpy, mainwin);
    XFreePixmap(dpy, bgpm);
    return None;
  }
  if(myXAllocNamedColor(dpy, attr.colormap, "#aad5c3", &scr1, &exa) &&
     myXAllocNamedColor(dpy, attr.colormap, "#081052", &scr2, &exa)) {
    gcv.background = scr1.pixel;
    gcv.foreground = scr2.pixel;
  } else {
    fprintf(stderr, "Failed to allocate LCD colors\n");
    XDestroyWindow(dpy, mainwin);
    XFreePixmap(dpy, bgpm);
    return None;
  }
  maingc = XCreateGC(dpy, mainwin, GCForeground|GCBackground, &gcv);
  XSelectInput(dpy, mainwin, ExposureMask|KeyPressMask|KeyReleaseMask);
  XMapRaised(dpy, mainwin);  
#ifdef USE_SDL_AUDIO
  init_sound();
#endif
  return mainwin;
}

void vmputpixel(int x, int y, int p)
{
  if(pixdbl) {
    x<<=1;
    y<<=1;
    XPutPixel(mainimg, x, y, p&1);
    XPutPixel(mainimg, x+1, y, p&1);
    XPutPixel(mainimg, x, y+1, p&1);
    XPutPixel(mainimg, x+1, y+1, p&1);
  } else
    XPutPixel(mainimg, x, y, p&1);
}

void low_redrawlcd()
{
  if(pixdbl)
    XPutImage(maindpy, mainwin, maingc, mainimg, 0, 0, 33, 86, 96, 80);
  else
    XPutImage(maindpy, mainwin, maingc, mainimg, 0, 0, 17, 43, 48, 40);
}

void redrawlcd()
{
  low_redrawlcd();
  XFlush(maindpy);
}

void checkevents()
{
  XEvent event;

  if(QLength(maindpy)==0) {
    struct timeval t;
    fd_set rfds;
    int x_fd=ConnectionNumber(maindpy);
    FD_ZERO(&rfds);
    FD_SET(x_fd, &rfds);
    t.tv_sec = t.tv_usec = 0;
    if (select(x_fd+1, &rfds, NULL, NULL, &t) > 0) {
      if(FD_ISSET(x_fd, &rfds))
	XPeekEvent(maindpy, &event);
    }
  }
  while(QLength(maindpy)>0) {
    XNextEvent(maindpy, &event);
    switch(event.type) {
      case Expose:
	if(!event.xexpose.count)
	  low_redrawlcd();
	break;
      case KeyPress:
	switch(XLookupKeysym(&event.xkey, 0)) {
	 case XK_Up:
	   keypress(0);
	   break;
	 case XK_Down:
	   keypress(1);
	   break;
	 case XK_Left:
	   keypress(2);
	   break;
	 case XK_Right:
	   keypress(3);
	   break;
	 case 'a':
	 case 'A':
	   keypress(4);
	   break;
	 case 'b':
	 case 'B':
	   keypress(5);
	   break;
	 case 'm':
	 case 'M':
	 case XK_Return:
	   keypress(6);
	   break;
	 case 's':
	 case 'S':
	 case ' ':
	   keypress(7);
	   break;
	}
	break;
     case KeyRelease:
	switch(XLookupKeysym(&event.xkey, 0)) {
	 case XK_Up:
	   keyrelease(0);
	   break;
	 case XK_Down:
	   keyrelease(1);
	   break;
	 case XK_Left:
	   keyrelease(2);
	   break;
	 case XK_Right:
	   keyrelease(3);
	   break;
	 case 'a':
	 case 'A':
	   keyrelease(4);
	   break;
	 case 'b':
	 case 'B':
	   keyrelease(5);
	   break;
	 case 'm':
	 case 'M':
	 case XK_Return:
	   keyrelease(6);
	   break;
	 case 's':
	 case 'S':
	 case ' ':
	   keyrelease(7);
	   break;
	}
        break;
    }
  }
}

void waitforevents(struct timeval *t)
{
  XEvent event;

  XFlush(maindpy);
  while(QLength(maindpy)==0) {
    fd_set rfds;
    int x_fd=ConnectionNumber(maindpy);
    FD_ZERO(&rfds);
    FD_SET(x_fd, &rfds);
    if (select(x_fd+1, &rfds, NULL, NULL, t) > 0) {
      if(FD_ISSET(x_fd, &rfds))
	XPeekEvent(maindpy, &event);
    }
    if(t != NULL)
      return;
  }
}

void sound(int freq)
{
  sound_freq = freq;
}
