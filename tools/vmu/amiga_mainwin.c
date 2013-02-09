#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <devices/timer.h>
#include <devices/inputevent.h>
#include <devices/ahi.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#ifdef __GNUC__
#define CONSTRUCTOR(x) static __attribute__((constructor)) void c_##x()
#define DESTRUCTOR(x) static __attribute__((destructor)) void d_##x()
#define CONSTR_FAIL exit(1)
#define CONSTR_SUCCEED return
#else
#include <constructor.h>
#define CONSTR_FAIL return 1
#define CONSTR_SUCCEED return 0
#endif
#include "prototypes.h"

#include "vmu.xpm"
#include "vmu.5.xpm"

#define WORDSPERLINE (8)
#define PUTPIXEL(i, x, y, p) do { \
    if((p)) \
      (i)[(y)*WORDSPERLINE+((x)>>4)] |= 1<<(15&~(x)); \
    else \
      (i)[(y)*WORDSPERLINE+((x)>>4)] &= ~(1<<(15&~(x))); \
  } while(0)

#ifdef __PPC__
#define __near
#define __regargs
#endif

__near long __oslibversion = 37;

__near static struct Window *mainwin = NULL;
__near static WORDBITS *mainimg = NULL;
__near static struct Screen *maindpy = NULL;
__near static struct BitMap *bgimg = NULL;
__near static LONG myfgpen = -1, mybgpen = -1;
__near static LONG pixel[256];
__near static struct Library *TimerBase = NULL;
__near static struct MsgPort *TimePort = NULL;
__near static struct timerequest *TimeReq = NULL;
__near static struct Library *KeymapBase = NULL;
#ifdef __amigaos4__
__near static struct KeymapIFace *IKeymap = NULL;
__near static struct AHIIFace *IAHI = NULL;
#endif
__near static struct Library *AHIBase = NULL;
__near static struct MsgPort *ahiport = NULL;
__near static struct IORequest *ahireq = NULL;
__near static struct AHIAudioCtrl *ahictrl = NULL;
__near static BOOL ahiopen = FALSE;

__near int pixdbl = 0;

void __regargs _CXBRK(void)
{
  exit(20);
}

static void close_ahi()
{
  if(ahictrl) {
    AHI_FreeAudio(ahictrl);
    ahictrl = NULL;
  }
#ifdef __amigaos4__
  if(IAHI) {
    DropInterface((struct Interface *)IAHI);
    IAHI = NULL;
  }
#endif
  if(ahiopen) {
    CloseDevice(ahireq);
    ahiopen = 0;
    AHIBase = NULL;
  }
  if(ahireq) {
    DeleteIORequest(ahireq);
    ahireq = NULL;
  }
  if(ahiport) {
    DeleteMsgPort(ahiport);
    ahiport = NULL;
  }
}

static BOOL open_ahi()
{
  static signed char sqr_wave[] = { -0x80, 0x7f };
  static struct AHISampleInfo info = {
    AHIST_M8S,
    sqr_wave,
    sizeof(sqr_wave)
  };

  if((ahiport = CreateMsgPort()) &&
     (ahireq = CreateIORequest(ahiport, sizeof(struct AHIRequest)))) {
    ((struct AHIRequest *)ahireq)->ahir_Version = 4;
    if(!OpenDevice(AHINAME, AHI_NO_UNIT, ahireq, 0)) {
      ahiopen = TRUE;
      AHIBase = (struct Library *)ahireq->io_Device;
      if(
#ifdef __amigaos4__
         (IAHI = (struct AHIIFace *)GetInterface(AHIBase, "main", 1, 0)) &&
#endif
	 (ahictrl = AHI_AllocAudio(AHIA_Channels, 1, AHIA_Sounds, 1, TAG_END)) &&
         !AHI_LoadSound(0, AHIST_SAMPLE, &info, ahictrl))
        return TRUE;
    }
  }
  close_ahi();
  return FALSE;
}

void sound(int freq)
{
  if(!ahiopen)
    return;

  if(freq < 0) {
    AHI_ControlAudio(ahictrl, AHIC_Play, FALSE, TAG_END);
    return;
  }

  AHI_Play(ahictrl, AHIP_BeginChannel, 0, AHIP_Freq, freq*2, AHIP_Pan, 0x8000, AHIP_Vol, 0x10000, AHIP_Sound, 0, AHIP_EndChannel, 0, TAG_END);
  AHI_ControlAudio(ahictrl, AHIC_Play, TRUE, TAG_END);
}


CONSTRUCTOR(mainwin)
{
  memset(pixel, ~0, sizeof(pixel));
  if((KeymapBase = OpenLibrary("keymap.library", 37L))) {
#ifdef __amigaos4__
    if((IKeymap = (struct KeymapIFace *)GetInterface(KeymapBase, "main", 1, 0))) {
#endif
      if((TimePort = CreateMsgPort())) {
        if((TimeReq = (struct timerequest *)CreateIORequest(TimePort, sizeof(struct timerequest)))) {
          if(!OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)TimeReq, 0)) {
            TimerBase=(struct Library *)TimeReq->tr_node.io_Device;
            CONSTR_SUCCEED;
          }
          DeleteIORequest((struct IORequest *)TimeReq);
          TimeReq = NULL;
        }
        DeleteMsgPort(TimePort);
        TimePort = NULL;
      }
#ifdef __amigaos4__
      DropInterface((struct Interface *)IKeymap);
      IKeymap = NULL;
    }
#endif
    CloseLibrary(KeymapBase);
    KeymapBase = NULL;
  }
  CONSTR_FAIL;
}

DESTRUCTOR(mainwin)
{
  int i;

  close_ahi();
  if(GfxBase->LibNode.lib_Version >= 39) {
    if(myfgpen >= 0)
      ReleasePen(maindpy->ViewPort.ColorMap, myfgpen);
    if(mybgpen >= 0)
      ReleasePen(maindpy->ViewPort.ColorMap, mybgpen);
    for(i=0; i<256; i++)
      if(pixel[i] >= 0)
        ReleasePen(maindpy->ViewPort.ColorMap, pixel[i]);
  }
  if(bgimg != NULL)
    FreeBitMap(bgimg);
  if(mainimg != NULL)
    FreeVec(mainimg);
  if(mainwin != NULL)
    CloseWindow(mainwin);
  if(maindpy != NULL)
    UnlockPubScreen(NULL, maindpy);
  if(TimerBase) {
    CloseDevice((struct IORequest *)TimeReq);
    TimerBase = NULL;
  }
  if(TimeReq) {
    DeleteIORequest((struct IORequest *)TimeReq);
    TimeReq = NULL;
  }
  if(TimePort) {
    DeleteMsgPort(TimePort);
    TimePort = NULL;
  }
#ifdef __amigaos4__
  if(IKeymap) {
    DropInterface((struct Interface *)IKeymap);
    IKeymap = NULL;
  }
#endif
  if(KeymapBase) {
    CloseLibrary(KeymapBase);
    KeymapBase = NULL;
  }
}

void error_msg(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);
}

static struct BitMap *mkbgpixmap(struct Screen *dpy, char **vmu1_xpm, int *wret, int *hret)
{
  int i, w, h, nc, x, y;
  struct BitMap *img;
  struct RastPort rp;
  sscanf(vmu1_xpm[0], "%d %d %d", &w, &h, &nc);
  for(i=1; i<=nc; i++) {
    int r, g, b;
    sscanf(vmu1_xpm[i]+4, "#%2x%2x%2x", &r, &g, &b);
    if(GfxBase->LibNode.lib_Version < 39)
      pixel[*(unsigned char *)vmu1_xpm[i]] = (r+g+b>300? 2:1);
    else if((pixel[*(unsigned char *)vmu1_xpm[i]] = 
            ObtainBestPen(maindpy->ViewPort.ColorMap, 0x1010101UL*r, 0x1010101UL*g, 0x1010101UL*b,
		      OBP_Precision, PRECISION_ICON, TAG_END)) < 0) {
      fprintf(stderr, "Failed to allocate color %s\n", vmu1_xpm[i]+4);
      return NULL;
    }
  }
  if((img = AllocBitMap(w, h, dpy->RastPort.BitMap->Depth, 0, dpy->RastPort.BitMap))==NULL) {
      fprintf(stderr, "Failed to allocate BitMap\n");
      return NULL;
  }
  InitRastPort(&rp);
  rp.BitMap = img;
  SetDrMd(&rp, JAM1);
  for(y=0; y<h; y++) {
    unsigned char *p = (unsigned char *)vmu1_xpm[i++];
    for(x=0; x<w; x++) {
      SetAPen(&rp, pixel[*p++]);
      WritePixel(&rp, x, y);
    }
  }
  *wret = w;
  *hret = h;
  return img;
}

static WORDBITS *mkmainimg(struct Screen *dpy, struct Window *win, int w, int h)
{
  WORDBITS *img;

  if((img = AllocVec(h*WORDSPERLINE*2, MEMF_CHIP|MEMF_CLEAR))==NULL) {
    fprintf(stderr, "Failed to allocate image\n");
    return NULL;
  }
  return img;
}

struct Window *openmainwin(struct Screen *dpy)
{
  int w, h;

  maindpy = dpy;
  if((bgimg = mkbgpixmap(dpy, (pixdbl? vmu_large:vmu_small), &w, &h))==NULL)
    return NULL;
  mainwin = OpenWindowTags(NULL, WA_PubScreen, dpy, WA_InnerWidth, w, WA_InnerHeight, h,
			   WA_GimmeZeroZero, TRUE, WA_SmartRefresh, TRUE,
			   WA_Title, "SoftVMS", WA_DragBar, TRUE, WA_CloseGadget, TRUE,
			   WA_DepthGadget, TRUE, WA_Activate, TRUE,
			   WA_IDCMP, IDCMP_RAWKEY|IDCMP_CLOSEWINDOW, TAG_END);
  if(mainwin == NULL)
    return NULL;
  BltBitMapRastPort(bgimg, 0, 0, mainwin->RPort, 0, 0, w, h, 0xc0);
  if((mainimg = mkmainimg(dpy, mainwin, (pixdbl? 96:48), (pixdbl? 80:40)))==NULL)
    return NULL;
  SetAPen(mainwin->RPort, mainwin->DetailPen);
  SetBPen(mainwin->RPort, mainwin->BlockPen);
  SetDrMd(mainwin->RPort, JAM2|INVERSVID);
  if(GfxBase->LibNode.lib_Version >= 39) {
    if((myfgpen = ObtainBestPen(maindpy->ViewPort.ColorMap, 0xaaaaaaaaUL, 0xd5d5d5d5UL, 0xc3c3c3c3UL,
				OBP_Precision, PRECISION_GUI, TAG_END))>=0 &&
       (mybgpen = ObtainBestPen(maindpy->ViewPort.ColorMap, 0x08080808UL, 0x10101010UL, 0x52525252UL,
				OBP_Precision, PRECISION_GUI, TAG_END))>=0) {
      SetAPen(mainwin->RPort, myfgpen);
      SetBPen(mainwin->RPort, mybgpen);
    } else {
      fprintf(stderr, "Failed to allocate LCD colors\n");
      return NULL;
    }
  }
  FreeBitMap(bgimg);
  bgimg = NULL;
  if(!open_ahi())
    fprintf(stderr, "Failed to open ahi.device, sound disabled.\n");
  return mainwin;
}

void vmputpixel(int x, int y, int p)
{
  if(pixdbl) {
    x<<=1;
    y<<=1;
    PUTPIXEL(mainimg, x, y, p&1);
    PUTPIXEL(mainimg, x+1, y, p&1);
    PUTPIXEL(mainimg, x, y+1, p&1);
    PUTPIXEL(mainimg, x+1, y+1, p&1);
  } else
    PUTPIXEL(mainimg, x, y, p&1);
}

void redrawlcd()
{
  if(pixdbl)
    BltTemplate((PLANEPTR)mainimg, 0, WORDSPERLINE*2, mainwin->RPort, 33, 86, 96, 80);
  else
    BltTemplate((PLANEPTR)mainimg, 0, WORDSPERLINE*2, mainwin->RPort, 17, 43, 48, 40);
}

void checkevents()
{
  struct Message *msg;
  struct IntuiMessage *event;
  struct InputEvent ie;
  int cw = 0;
  char buffer;

  ie.ie_Class = IECLASS_RAWKEY;
  ie.ie_SubClass = 0;
  while((msg = GetMsg(mainwin->UserPort))) {
    event = (struct IntuiMessage *)msg;
    switch(event->Class) {
      case IDCMP_CLOSEWINDOW:
	cw = 1;
	break;
      case IDCMP_RAWKEY:
	ie.ie_Code = event->Code;
	ie.ie_Qualifier = event->Qualifier;
	/* recover dead key codes & qualifiers */
	ie.ie_EventAddress = (APTR *) *((ULONG *)event->IAddress);
	if(1==MapRawKey(&ie, &buffer, 1, 0))
          switch(buffer) {
	    case 'a':
	    case 'A':
	      ie.ie_Code = 0x20;
	      break;
	    case 'b':
	    case 'B':
	      ie.ie_Code = 0x35;
	      break;
	    case 'm':
	    case 'M':
	    case '\r':
	    case '\n':
	      ie.ie_Code = 0x44;
	      break;
	    case 's':
	    case 'S':
	    case ' ':
	      ie.ie_Code = 0x40;
	      break;
          }
	ie.ie_Code |= (event->Code & 0x80);
	switch(ie.ie_Code) {
	 case 0x4c:
	   keypress(0);
	   break;
	 case 0x4d:
	   keypress(1);
	   break;
	 case 0x4f:
	   keypress(2);
	   break;
	 case 0x4e:
	   keypress(3);
	   break;
	 case 0x20:
	   keypress(4);
	   break;
	 case 0x35:
	   keypress(5);
	   break;
	 case 0x37:
	 case 0x44:
	   keypress(6);
	   break;
	 case 0x21:
	 case 0x40:
	   keypress(7);
	   break;
	 case 0xcc:
	   keyrelease(0);
	   break;
	 case 0xcd:
	   keyrelease(1);
	   break;
	 case 0xcf:
	   keyrelease(2);
	   break;
	 case 0xce:
	   keyrelease(3);
	   break;
	 case 0xa0:
	   keyrelease(4);
	   break;
	 case 0xb5:
	   keyrelease(5);
	   break;
	 case 0xb7:
	 case 0xc4:
	   keyrelease(6);
	   break;
	 case 0xa1:
	 case 0xc0:
	   keyrelease(7);
	   break;
	}
        break;
    }
    ReplyMsg(msg);
  }
  if(cw || (SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C))
    _CXBRK();
}

void waitforevents(struct timeval *t)
{
  ULONG mask;
  if(t) {
    if(t->tv_secs == 0 && t->tv_micro == 0)
      return;
    TimeReq->tr_node.io_Command = TR_ADDREQUEST;
    TimeReq->tr_time = *t;
    SendIO((struct IORequest *)TimeReq);
  }
  mask = Wait((1UL<<mainwin->UserPort->mp_SigBit)|(1UL<<TimePort->mp_SigBit)|
		SIGBREAKF_CTRL_C);
  if(t && (mask&(1UL<<TimePort->mp_SigBit))) {
    WaitIO((struct IORequest *)TimeReq);
    t = NULL;
  }
  if(t) {
    AbortIO((struct IORequest *)TimeReq);
    WaitIO((struct IORequest *)TimeReq);
  }
  if(mask & SIGBREAKF_CTRL_C)
    _CXBRK();
}

#ifndef __amigaos4__
/* UNIX compat... */

int gettimeofday (struct timeval *tv, struct timezone *tz)
{
  return timer((unsigned int *)tv);
}

void __stdargs sleep(unsigned t)
{
  Delay(t*50);
}
#endif

