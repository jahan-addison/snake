#include <stdio.h>
#include <stdlib.h>

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

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "prototypes.h"


struct instance {
  unsigned char hdr[16], tm[4];
  int rstate, rcnt, roffs, complete;
  int lcdi_width, lcdi_height, lcdi_nframes, fcntr;
  int childpid;
  long fsz, *lcdi_frametime;
  unsigned char **lcdi_framedata;

  Display *dpy;
  Window window;
  Colormap cmap;
  Visual *visual;
};

char *NP_GetMIMEDescription(void)
{
  /*fprintf(stderr, "NP_GetMIMEDescription\n");*/
  return "application/x-dreamcast-lcdimg: lcd: Dreamcast LCD Image;";
}

short NP_GetValue(void *instance, int variable, void *value)
{
  /*fprintf(stderr, "NP_GetValue %d\n", variable);*/
  switch(variable) {
  case 1:
    *((char **)value) = "Dreamcast LCDImage";
    break;
  case 2:
    *((char **)value) = "A silly little plugin to display .LCD images";
    break;
  default:
    return 9;
  }
  return 0;
}

static short lcdnew(char *pluginType, void **instance, unsigned short mode, short argc,
		    char* argn[], char* argv[], void* saved)
{
  int i;
  /*fprintf(stderr, "New(%s, %p, %d, %d, %p, %p, %p)\n", pluginType, instance, mode,
    argc, argn, argv, saved);
  for(i=0; i<argc; i++)
    fprintf(stderr, "argn[%d]=%s, argv[%d]=%s\n", i, argn[i], i, argv[i]);*/
  *instance = calloc(1, sizeof(struct instance));
  return 0;
} 

static void free_framedata(struct instance *inst)
{
  int i;
  for(i=0; i<inst->lcdi_nframes; i++)
    if(inst->lcdi_framedata[i] != NULL)
      free(inst->lcdi_framedata[i]);
  free(inst->lcdi_framedata);
  inst->lcdi_framedata = NULL;
}

static short destroy(void **instance, void** save)
{
  /*fprintf(stderr, "Destroy(%p, %p)\n", instance, save);*/
  *save = NULL;
  if(*instance != NULL) {
    struct instance *i = *instance;

    if(i->childpid > 0) {
      kill(i->childpid, 15);
      waitpid(i->childpid, NULL, 0);
    }

    if(i->lcdi_frametime != NULL)
      free(i->lcdi_frametime);
    if(i->lcdi_framedata)
      free_framedata(i);
    free(i);
  }
  return 0;
}

static Status myXAllocNamedColor(struct instance *i, char *color_name,
				 XColor *screen_def, XColor *exact_def)
{
  Status s;

  if((s=XAllocNamedColor(i->dpy, i->cmap, color_name, screen_def, exact_def)))
    return s;
  if((s=XLookupColor(i->dpy, i->cmap, color_name, screen_def, exact_def))||
     (s=XParseColor(i->dpy, i->cmap, color_name, exact_def))) {
    *screen_def = *exact_def;
    screen_def->pixel = 0;
    screen_def->flags = DoRed|DoGreen|DoBlue;
    s=XAllocColor(i->dpy, i->cmap, screen_def);
  }
  return s;
}

static void draw(struct instance *i)
{
  int x, y, w = i->lcdi_width, h = i->lcdi_height;
  int bpl = (w + 31) & ~31;
  XImage *img;
  XGCValues val;
  XColor exa, scr1, scr2;
  GC gc;
  struct timeval tnext;

  if(!i->window || !i->complete || !(i->dpy = XOpenDisplay(NULL)))
    return;

  if((img = XCreateImage(i->dpy, i->visual, 1, XYBitmap, 0,
			 malloc(bpl*h), w, h, 32, bpl)) == NULL)
    return;

  memset(&val, 0, sizeof(val));
  if(myXAllocNamedColor(i, "#aad5c3", &scr1, &exa) &&
     myXAllocNamedColor(i, "#081052", &scr2, &exa)) {
    val.background = scr1.pixel;
    val.foreground = scr2.pixel;
  } else {
    XWindowAttributes attr;
    XGetWindowAttributes(i->dpy, i->window, &attr);
    val.background = WhitePixelOfScreen(attr.screen);
    val.foreground = BlackPixelOfScreen(attr.screen);
  }
  gc = XCreateGC(i->dpy, i->window, GCForeground|GCBackground, &val);

  XSelectInput(i->dpy, i->window, ExposureMask);

  GETTIMEOFDAY(&tnext);
  if(i->fcntr < i->lcdi_nframes) {
    tnext.tv_sec += i->lcdi_frametime[i->fcntr]/1000;
    if((tnext.tv_usec += (i->lcdi_frametime[i->fcntr]%1000)*1000)>=1000000) {
      tnext.tv_sec++;
      tnext.tv_usec -= 1000000;
    }
  }

  for(;;) {

    XEvent ev;

    if(i->fcntr < i->lcdi_nframes) {
      unsigned char *d = i->lcdi_framedata[i->fcntr];
      for(y=0; y<h; y++)
	for(x=0; x<w; x++)
	  XPutPixel(img, x, y, !!*d++);
    } else
      for(y=0; y<h; y++)
	for(x=0; x<w; x++)
	  XPutPixel(img, x, y, 0);
    
    XPutImage(i->dpy, i->window, gc, img, 0, 0, 0, 0,
	      i->lcdi_width, i->lcdi_height);

    XFlush(i->dpy);
    while(QLength(i->dpy)==0) {
      fd_set rfds;
      int x_fd=ConnectionNumber(i->dpy);
      struct timeval t, now;

      if(i->lcdi_nframes > 1) {
	GETTIMEOFDAY(&now);
	if(now.tv_sec>tnext.tv_sec ||
	   (now.tv_sec == tnext.tv_sec && now.tv_usec >= tnext.tv_usec)) {
	  if(++i->fcntr >= i->lcdi_nframes)
	    i->fcntr = 0;
	  tnext.tv_sec += i->lcdi_frametime[i->fcntr]/1000;
	  if((tnext.tv_usec += (i->lcdi_frametime[i->fcntr]%1000)*1000)>=1000000) {
	    tnext.tv_sec++;
	    tnext.tv_usec -= 1000000;
	  }
	  break;
	} else if(tnext.tv_usec<now.tv_usec) {
	  t.tv_usec = 1000000 + tnext.tv_usec - now.tv_usec;
	  t.tv_sec = tnext.tv_sec - now.tv_sec - 1;
	} else {
	  t.tv_usec = tnext.tv_usec - now.tv_usec;
	  t.tv_sec = tnext.tv_sec - now.tv_sec;
	}
      } else {
	t.tv_usec = 0;
	t.tv_sec = 1000;
      }
      FD_ZERO(&rfds);
      FD_SET(x_fd, &rfds);
      if (select(x_fd+1, &rfds, NULL, NULL, &t) > 0) {
	if(FD_ISSET(x_fd, &rfds))
	  XPeekEvent(i->dpy, &ev);
      }
    }

    while(QLength(i->dpy)>0)
      XNextEvent(i->dpy, &ev);
  }
}

static void start_drawing(struct instance *i)
{
  if(i->childpid > 0) {
    kill(i->childpid, 15);
    waitpid(i->childpid, NULL, 0);
  }
  if((i->childpid = fork()) == 0) {
    draw(i);
    _exit(0);
  }
}

static short setwindow(void **instance, void* window)
{
  struct instance *i = *instance;
  struct {
    Window win;
    INT32 x, y;
    CARD32 w, h;
    struct { CARD16 top, left, bottom, right; } cliprect;
    void *ws_info;
  } *win = window;
  struct {
    INT32 type;
    Display *dpy;
    Visual *vis;
    Colormap cmap;
    unsigned int depth;
  } *info = win->ws_info;

  /*fprintf(stderr, "Setwindow(%p, %p)\n", instance, window);*/
  if(win->win == i->window)
    return 0;
  i->dpy = info->dpy;
  i->window = win->win;
  i->cmap = info->cmap;
  i->visual = info->vis;
  if(i->complete)
    start_drawing(i);
  return 0;
}

static short newstream(void **instance, char *type, void* stream, unsigned char seekable,
		       unsigned short *stype)
{
  struct instance *i = *instance;
  /*fprintf(stderr, "Newstream(%p, %s, %p, %d, %p)\n", instance, type, stream, seekable, stype);*/
  if(i->lcdi_frametime != NULL) {
    free(i->lcdi_frametime);
    i->lcdi_frametime = NULL;
  }
  if(i->lcdi_framedata)
    free_framedata(i);
  i->rstate = i->roffs = i->complete = 0;
  i->rcnt = 16;
  i->lcdi_width = i->lcdi_height = i->lcdi_nframes = i->fcntr = 0;
  *stype = 3;
  return 0;
}

static short destroystream(void **instance, void* stream, short reason)
{
  /*fprintf(stderr, "Destroystream(%p, %p, %d)\n", instance, stream, reason);*/
  return 0;
}

static void asfile(void **instance, void* stream, const char* fname)
{
  /*fprintf(stderr, "Asfile(%p, %p, %p)\n", instance, stream, fname);
  if(fname)
    fprintf(stderr, "File is %s\n", fname);
  */
}

static int writeready(void **instance, void* stream)
{
  /*  fprintf(stderr, "Writeready(%p, %p)\n", instance, stream); */
  return ((struct instance *)*instance)->rcnt;
}

static int write(void **instance, void* stream, int offset, int len, void* buffer)
{
  int n;
  struct instance *i = *instance;
  unsigned char *hdr = i->hdr;
  unsigned char *b = buffer;
  /*fprintf(stderr, "Write(%p, %p, %d, %d, %p)\n", instance, stream, offset, len, buffer);*/
  if(i->complete)
    return 0;
  if(len > i->rcnt)
    len = i->rcnt;
  if(len <= 0)
    return len;
  switch(i->rstate) {
  case 0:
    memcpy(i->hdr+i->roffs, buffer, len);
    break;
  case 1:
    for(n=i->roffs; n<i->roffs+len; n++) {
      i->tm[n&3] = *b++;
      if((n&3) == 3)
	i->lcdi_frametime[n>>2] =
	  i->tm[0]+(i->tm[1]<<8)+(i->tm[2]<<16)+(i->tm[3]<<24);
    }
    break;
  case 2:
    memcpy(i->lcdi_framedata[i->fcntr]+i->roffs, buffer, len);
    break;
  }
  i->roffs += len;
  if((i->rcnt -= len) == 0) {
    i->roffs = 0;
    switch(++i->rstate) {
    case 1:
      if(strncmp(hdr, "LCDi", 4))
	break;
      i->lcdi_width = hdr[6]+(hdr[7]<<8);
      i->lcdi_height = hdr[8]+(hdr[9]<<8);
      i->lcdi_nframes = hdr[14]+(hdr[15]<<8);
      i->fsz = i->lcdi_width * i->lcdi_height;
      if((i->lcdi_frametime = calloc(i->lcdi_nframes, sizeof(long)))==NULL ||
	 (i->lcdi_framedata = calloc(i->lcdi_nframes, sizeof(unsigned char *)))==NULL)
	break;
      i->rcnt = i->lcdi_nframes * 4;
      break;
    case 3:
      i->fcntr++;
      i->rstate = 2;
      /* FALLTHRU */
    case 2:
      if(i->fcntr < i->lcdi_nframes) {
	if((i->lcdi_framedata[i->fcntr] = malloc(i->fsz))==NULL)
	  break;
	i->rcnt = i->fsz;
      } else {
	i->complete = 1;
	i->fcntr = 0;
	/*fprintf(stderr, "Complete, window = %x\n", i->window);*/
	if(i->window)
	  start_drawing(i);
      }
      break;
    }
  }
  return len;
}

static short event(void **instance, void* event)
{
  /*fprintf(stderr, "Event(%p, %p)\n", instance, event);*/
  return 0;
}

static struct NPPluginFuncs {
  unsigned short size;
  unsigned short version;
  short (*newp)(char *pluginType, void **instance, unsigned short mode, short argc,
		char* argn[], char* argv[], void* saved);
  short (*destroy)(void **instance, void** save);
  short (*setwindow)(void **instance, void* window);
  short (*newstream)(void **instance, char *type, void* stream, unsigned char seekable,
		     unsigned short *stype);
  short (*destroystream)(void **instance, void* stream, short reason);
  void (*asfile)(void **instance, void* stream, const char* fname);
  int (*writeready)(void **instance, void* stream);
  int (*write)(void **instance, void* stream, int offset, int len, void* buffer);
  void (*print)(void **instance, void* platformPrint);
  short (*event)(void **instance, void* event);
  void (*urlnotify)(void **instance, const char* url, short reason, void* notifyData);
  void *javaClass;
} NPPluginFuncs = {
  sizeof(struct NPPluginFuncs),
  9,
  lcdnew,
  destroy,
  setwindow,
  newstream,
  destroystream,
  asfile,
  writeready,
  write,
  NULL,
  event,
  NULL,
  NULL
};

static struct NPNetscapeFuncs {
  unsigned short size;
  unsigned short version;
  /*
  NPN_GetURLUPP geturl;
  NPN_PostURLUPP posturl;
  NPN_RequestReadUPP requestread;
  NPN_NewStreamUPP newstream;
  NPN_WriteUPP write;
  NPN_DestroyStreamUPP destroystream;
  NPN_StatusUPP status;
  NPN_UserAgentUPP uagent;
  NPN_MemAllocUPP memalloc;
  NPN_MemFreeUPP memfree;
  NPN_MemFlushUPP memflush;
  NPN_ReloadPluginsUPP reloadplugins;
  NPN_GetJavaEnvUPP getJavaEnv;
  NPN_GetJavaPeerUPP getJavaPeer;
  NPN_GetURLNotifyUPP geturlnotify;
  NPN_PostURLNotifyUPP posturlnotify;
  NPN_GetValueUPP getvalue;
  */
} NPNetscapeFuncs;


short NP_Initialize(struct NPNetscapeFuncs *nsTable, struct NPPluginFuncs *pluginFuncs)
{
  /*fprintf(stderr, "NP_Initialize\n");*/
  if(nsTable == NULL || pluginFuncs == NULL)
    return 3;
  if(nsTable->version > 255)
    return 8;
  if(nsTable->size < sizeof(struct NPNetscapeFuncs))
    return 3;
  if(pluginFuncs->size < sizeof(struct NPPluginFuncs))
    return 3;
  NPNetscapeFuncs = *nsTable;
  *pluginFuncs = NPPluginFuncs;
  return 0;
}

short NP_Shutdown()
{
  /*fprintf(stderr, "NP_Shutdown\n");*/
  return 0;
}
