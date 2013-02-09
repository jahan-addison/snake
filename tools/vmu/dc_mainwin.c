#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include <ronin/ta.h>
#include <ronin/maple.h>
#include <ronin/dc_time.h>
#include <ronin/sincos_rroot.h>
#include <ronin/vmsfs.h>
#include <ronin/sound.h>
#include <ronin/soundcommon.h>

#include "vmu.xpm"

#define VMSCNT 8

#define SCALE 4
#define SC_WIDTH 48
#define SC_HEIGHT 32
#define BG_WIDTH 156
#define BG_HEIGHT 267
#define BG_XOFFS 33
#define BG_YOFFS 86

#define BG_LEFT ((640.0-BG_WIDTH*SCALE/2)/2)
#define BG_TOP ((480.0-BG_HEIGHT*SCALE/2)/2)
#define BG_RIGHT (BG_LEFT+BG_WIDTH*SCALE/2)
#define BG_BOTTOM (BG_TOP+BG_HEIGHT*SCALE/2)
#define SC_LEFT (BG_LEFT+BG_XOFFS*SCALE/2)
#define SC_TOP (BG_TOP+BG_YOFFS*SCALE/2)
#define SC_RIGHT (SC_LEFT+SC_WIDTH*SCALE)
#define SC_BOTTOM (SC_TOP+SC_HEIGHT*SCALE)
#define LC_LEFT (SC_LEFT-3)
#define LC_RIGHT (SC_RIGHT+2)
#define LC_TOP (SC_TOP-3)
#define LC_BOTTOM (SC_BOTTOM+7*SCALE)

static const unsigned char idLetters[4][96] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x7c, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x7e, 0x00, 0x00, 0xfe, 0x00,
    0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xef, 0x00, 0x00, 0xef, 0x01,
    0x00, 0xe7, 0x01, 0x80, 0xe7, 0x01, 0x80, 0xc7, 0x03, 0x80, 0xc7, 0x03,
    0x80, 0xc3, 0x03, 0xc0, 0xc3, 0x03, 0xc0, 0xff, 0x07, 0xc0, 0xff, 0x07,
    0xe0, 0xff, 0x07, 0xe0, 0xff, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x00, 0x0f,
    0xf0, 0x00, 0x0f, 0xf0, 0x00, 0x1f, 0xf0, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe0, 0xff, 0x01, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x07, 0xe0, 0xff, 0x0f,
    0xe0, 0x81, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x0f,
    0xe0, 0x81, 0x0f, 0xe0, 0xff, 0x07, 0xe0, 0xff, 0x01, 0xe0, 0xff, 0x03,
    0xe0, 0xff, 0x07, 0xe0, 0x81, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x0f,
    0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x81, 0x0f, 0xe0, 0xff, 0x07,
    0xe0, 0xff, 0x07, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xfc, 0x00, 0x00, 0xff, 0x01, 0x80, 0xff, 0x03, 0xc0, 0xff, 0x07,
    0xc0, 0x87, 0x0f, 0xe0, 0x03, 0x0f, 0xe0, 0x01, 0x0f, 0xf0, 0x01, 0x1e,
    0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00,
    0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x01, 0x0e,
    0xe0, 0x01, 0x0f, 0xe0, 0x03, 0x0f, 0xe0, 0x87, 0x0f, 0xc0, 0xff, 0x07,
    0x80, 0xff, 0x03, 0x00, 0xff, 0x01, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe0, 0xff, 0x00, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x07, 0xe0, 0xff, 0x07,
    0xe0, 0x81, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x1f, 0xe0, 0x01, 0x1e,
    0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1e,
    0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1e, 0xe0, 0x01, 0x1f,
    0xe0, 0x01, 0x0f, 0xe0, 0x01, 0x0f, 0xe0, 0x81, 0x0f, 0xe0, 0xff, 0x07,
    0xe0, 0xff, 0x03, 0xe0, 0xff, 0x01, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const unsigned char idDigits[2][96] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3e, 0x00,
    0xc0, 0x3f, 0x00, 0xc0, 0x3f, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0x3c, 0x00,
    0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00,
    0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00,
    0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00,
    0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0xff, 0x00, 0x80, 0xff, 0x01,
    0xc0, 0xff, 0x01, 0xc0, 0xe3, 0x03, 0xc0, 0xc1, 0x03, 0xe0, 0xc1, 0x03,
    0xe0, 0xc1, 0x03, 0x00, 0xc0, 0x03, 0x00, 0xe0, 0x03, 0x00, 0xe0, 0x01,
    0x00, 0xf0, 0x01, 0x00, 0xfc, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x1f, 0x00,
    0x80, 0x0f, 0x00, 0x80, 0x07, 0x00, 0xc0, 0x03, 0x00, 0xc0, 0xff, 0x03,
    0xe0, 0xff, 0x03, 0xe0, 0xff, 0x03, 0xe0, 0xff, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};


static void *bg_texture, *screen_texture[4+VMSCNT], *icon_texture[4];
static float bg_tex_w, bg_tex_h;
static int bg_tex_mode2;
static int current_buffer;
static int icon[4];
static int selected_vms = -1;
static char vmsexist[VMSCNT];
static float wobble = 1.0;

static int screen_filtering = 0;

static union { unsigned char c[32*64/2]; unsigned long l[32*64/8]; } mainimg;

static int twtab[1024];

static unsigned int icon_imagery[][22] = {
  {
    0x00092492, 0x00149249, 0x00149249, 0x000ffffe,
    0x00080002, 0x0038fff2, 0x00280002, 0x00280002,
    0x00281ff2, 0x00280002, 0x00280002, 0x0028fff2,
    0x00280002, 0x00280002, 0x0029fff2, 0x00280002,
    0x00280002, 0x002807f2, 0x00280002, 0x002ffffe,
    0x00200008, 0x003ffff8,
  },
  {
    0x0000ffc0, 0x00010020, 0x00020010, 0x00020c10,
    0x00040c08, 0x00041e08, 0x00041e08, 0x00043f08,
    0x00043f08, 0x00047f88, 0x0004ffc8, 0x0004ffc8,
    0x0004ffc8, 0x00046d88, 0x00040c08, 0x00040c08,
    0x00040c08, 0x00041e08, 0x00020010, 0x00020010,
    0x00010020, 0x0000ffc0,
  },
  {
    0x00001e00, 0x0000e1c0, 0x00030030, 0x00040c08,
    0x00080c04, 0x00080c04, 0x00100c02, 0x00100c02,
    0x00100c02, 0x00200001, 0x00200c01, 0x00200c01,
    0x00200001, 0x00101802, 0x00103002, 0x00106002,
    0x0008c004, 0x00088004, 0x00040008, 0x00030030,
    0x0000e1c0, 0x00001e00,
  },
  {
    0x00000800, 0x00000800, 0x00020c00, 0x00031e0e,
    0x00039e7c, 0x0003dff8, 0x0003fff0, 0x003fe3e0,
    0x000fe3f8, 0x0003e3ff, 0x0007e3fc, 0x000fe3f0,
    0x001fe3c0, 0x003fffe0, 0x0001e3f0, 0x0001e3f8,
    0x0001fffc, 0x0001df1e, 0x00018f00, 0x00010700,
    0x00000300, 0x00000100,
  },
};


void twiddleit(char **xpm, short *tex, int w, int h, int xadj)
{
  int x, y;
  for(y=0; y<h; y++) {
    unsigned char *p = xadj+(unsigned char *)*xpm++;
    unsigned short *txline = tex + twtab[y];
    for(x=0; x<w; x+=2) {
      short pp = *p++;
      pp |= (*p++)<<8;
      txline[twtab[x]>>1] = pp;
    }
  }  
}

static void *mkbgpixmap(char **vmu1_xpm, float *wret, float *hret, int *txret)
{
  int i, w, h, nc, txw, txh;
  int txmode = TA_POLYMODE2_BLEND_DEFAULT|TA_POLYMODE2_FOG_DISABLED|
    TA_POLYMODE2_BILINEAR_FILTER|TA_POLYMODE2_TEXTURE_MODULATE|
    TA_POLYMODE2_TEXTURE_CLAMP_U|TA_POLYMODE2_TEXTURE_CLAMP_V;
  short *tex;
  unsigned int (*pal)[4][256] = (unsigned int (*)[4][256])0xa05f9000;
  sscanf(vmu1_xpm[0], "%d %d %d", &w, &h, &nc);
  if(w&1) w++;
  for(txh=8; txh<h; txh<<=1, txmode+=1<<3);
  for(txw=8; txw<w; txw<<=1, txmode+=1<<0);
  tex = ta_txalloc(txh*txw);
  for(i=(txh*txw)>>1; --i; ) tex[i] = 0;
  (*pal)[0][0] = 0;
  for(i=1; i<=nc; i++) {
    int r, g, b;
    sscanf(vmu1_xpm[i]+4, "#%2x%2x%2x", &r, &g, &b);
    (*pal)[0][*(unsigned char *)vmu1_xpm[i]] = (r<<16)|(g<<8)|(b)|0xff000000;
  }
  if(txw > txh) {
    int bx;
    for(bx = 0; bx < w; bx += txh)
      twiddleit(vmu1_xpm+i, tex+bx*txh/2, (bx+txh>w? w-bx:txh), h, bx);    
  } else {
    int by;
    for(by = 0; by < h; by += txw)
      twiddleit(vmu1_xpm+i+by, tex+by*txw/2, w, (by+txw>h? h-by:txw), 0);
  }
  *wret = 1.0*w/txw;
  *hret = 1.0*h/txh;
  *txret = txmode;
  return tex;
}

static void *mkicontexture(unsigned int *imagery)
{
  int i, j;
  short *tex;
  tex = ta_txalloc(32*32/2);
  for(i=0; i<32*32/2; i++)
    tex[i] = 0;
  for(i=0; i<22; i+=2) {
    unsigned int l1 = *imagery++;
    unsigned int l2 = *imagery++;
    short *txline = tex + (twtab[i]>>1);
    for(j=0; j<16; j++) {
      txline[twtab[j]] =
	((l1&1)?1:0)|((l1&2)?0x10:0)|
	((l2&1)?0x100:0)|((l2&2)?0x1000:0);
      l1>>=2;
      l2>>=2;
    }
  }    
  return tex;
}

#define QACR0 (*(volatile unsigned int *)(void *)0xff000038)
#define QACR1 (*(volatile unsigned int *)(void *)0xff00003c)
void texture_memcpy64(void *dest, void *src, int cnt)
{
  unsigned int *s = (unsigned int *)src;
  unsigned int *d = (unsigned int *)(void *)
    (0xe0000000 | (((unsigned long)dest) & 0x03ffffc0));
  QACR0 = ((0xa4000000>>26)<<2)&0x1c;
  QACR1 = ((0xa4000000>>26)<<2)&0x1c;
  while(cnt--) {
    d[0] = *s++;
    d[1] = *s++;
    d[2] = *s++;
    d[3] = *s++;
    asm("pref @%0" : : "r" (s+16));
    d[4] = *s++;
    d[5] = *s++;
    d[6] = *s++;
    d[7] = *s++;
    asm("pref @%0" : : "r" (d));
    d += 8;
    d[0] = *s++;
    d[1] = *s++;
    d[2] = *s++;
    d[3] = *s++;
    asm("pref @%0" : : "r" (s+16));
    d[4] = *s++;
    d[5] = *s++;
    d[6] = *s++;
    d[7] = *s++;      
    asm("pref @%0" : : "r" (d));
    d += 8;
  }
}

#define GET_SXSY()							\
  float sx = 1.0, sy = 1.0, dx = 0.0, dy = 0.0;				\
  if(p < 1.0) {								\
    sx = 0.25;								\
    sy = 0.25;								\
    dx = (640.0/4 - (BG_RIGHT-BG_LEFT)/4)/2 - 0.25*BG_LEFT + 640.0/4*(i&3); \
    dy = (480.0/2 - (BG_BOTTOM-BG_TOP)/4)/2 - 0.25*BG_TOP + 480.0/2*(i>>2); \
    if(i == selected_vms)						\
      if(p == 0.0) {							\
	sx *= wobble;							\
	sy *= wobble;							\
	dx -= (wobble-1.0)*((BG_RIGHT-BG_LEFT)/4);			\
	dy -= (wobble-1.0)*((BG_BOTTOM-BG_TOP)/4)/2;			\
      } else {								\
	dx *= (1.0 - p);						\
	dy *= (1.0 - p);						\
	sx = p + sx * (1.0 - p);					\
	sy = p + sy * (1.0 - p);					\
      }									\
  }

static void draw_screen(float p)
{
  struct polygon_list mypoly;
  struct packed_colour_vertex_list myvertex;
  int i, alpha, alpha2;

  if(p < 1.0 && (alpha = 0xff-0x300*p) > 0) {
    alpha2 = (alpha<<24)|(((alpha*0xaa)>>8)<<16)|(((alpha*0xd5)>>8)<<8)|
      ((alpha*0xc3)>>8);
    alpha = (alpha<<24)|(alpha<<16)|(alpha<<8)|alpha;
  } else
    alpha = alpha2 = 0;

  texture_memcpy64(screen_texture[current_buffer], &mainimg,
		   sizeof(mainimg)/64);

  ta_begin_frame();

  for(i=0; i<VMSCNT; i++)
    if(i == selected_vms || (p < 1.0 && vmsexist[i] == 2)) {
      GET_SXSY();

      mypoly.cmd =
	TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_OPAQUE|TA_CMD_POLYGON_SUBLIST|
	TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR|
	TA_CMD_POLYGON_TEXTURED;
      mypoly.mode1 = TA_POLYMODE1_Z_GREATEREQUAL;
      mypoly.mode2 = bg_tex_mode2;
      mypoly.texture = TA_TEXTUREMODE_CLUT8|TA_TEXTUREMODE_CLUTBANK8(0)|
	TA_TEXTUREMODE_TWIDDLED|TA_TEXTUREMODE_ADDRESS(bg_texture);
      
      mypoly.red = mypoly.green = mypoly.blue = mypoly.alpha = 0;
      
      ta_commit_list(&mypoly);
      
      myvertex.cmd = TA_CMD_VERTEX;
      myvertex.colour = (i == selected_vms? 0xffffffff : alpha);
      myvertex.ocolour = 0;
      myvertex.z = (i == selected_vms? 0.5 : 0.25);
      myvertex.u = 0.0;
      myvertex.v = 0.0;
      
      myvertex.x = BG_LEFT*sx+dx;
      myvertex.y = BG_TOP*sy+dy;
      ta_commit_list(&myvertex);
      
      myvertex.x = BG_RIGHT*sx+dx;
      myvertex.v = bg_tex_w;
      ta_commit_list(&myvertex);
      
      myvertex.x = BG_LEFT*sx+dx;
      myvertex.y = BG_BOTTOM*sy+dy;
      myvertex.u = bg_tex_h;
      myvertex.v = 0.0;
      ta_commit_list(&myvertex);
      
      myvertex.x = BG_RIGHT*sx+dx;
      myvertex.v = bg_tex_w;
      myvertex.cmd |= TA_CMD_VERTEX_EOS;
      ta_commit_list(&myvertex);  
      
      mypoly.cmd =
	TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_OPAQUE|TA_CMD_POLYGON_SUBLIST|
	TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR;
      mypoly.mode1 = TA_POLYMODE1_Z_GREATEREQUAL;
      mypoly.mode2 = TA_POLYMODE2_BLEND_DEFAULT|TA_POLYMODE2_FOG_DISABLED;
      mypoly.texture = 0;
      
      ta_commit_list(&mypoly);

      myvertex.cmd = TA_CMD_VERTEX;
      myvertex.colour = (i == selected_vms? 0xffaad5c3 : alpha2);
      myvertex.z += 0.0625;

      myvertex.x = LC_LEFT*sx+dx;
      myvertex.y = LC_TOP*sy+dy;
      ta_commit_list(&myvertex);

      myvertex.x = LC_RIGHT*sx+dx;
      ta_commit_list(&myvertex);
      
      myvertex.x = LC_LEFT*sx+dx;
      myvertex.y = LC_BOTTOM*sy+dy;
      ta_commit_list(&myvertex);
      
      myvertex.x = LC_RIGHT*sx+dx;
      myvertex.cmd |= TA_CMD_VERTEX_EOS;
      ta_commit_list(&myvertex);
    }


  ta_commit_end();

  for(i=0; i<VMSCNT; i++)
    if(i == selected_vms || (p < 1.0 && vmsexist[i] == 2)) {
      GET_SXSY();

      mypoly.cmd =
	TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_TRANSPARENT|TA_CMD_POLYGON_SUBLIST|
	TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR|
	TA_CMD_POLYGON_TEXTURED;
      mypoly.mode1 = TA_POLYMODE1_Z_GREATEREQUAL;
      mypoly.mode2 = TA_POLYMODE2_BLEND_SRC_ALPHA|TA_POLYMODE2_BLEND_DST_INVALPHA|
	TA_POLYMODE2_FOG_DISABLED|TA_POLYMODE2_TEXTURE_MODULATE|
	TA_POLYMODE2_U_SIZE_32|TA_POLYMODE2_V_SIZE_64|TA_POLYMODE2_ENABLE_ALPHA|
	TA_POLYMODE2_TEXTURE_CLAMP_U|TA_POLYMODE2_TEXTURE_CLAMP_V|screen_filtering;
      mypoly.texture = TA_TEXTUREMODE_CLUT4|TA_TEXTUREMODE_CLUTBANK4(16)|
	TA_TEXTUREMODE_TWIDDLED|
	TA_TEXTUREMODE_ADDRESS(screen_texture[i == selected_vms && p > 0.0? current_buffer:i+4]);
      
      ta_commit_list(&mypoly);
      
      myvertex.cmd = TA_CMD_VERTEX;
      myvertex.colour = (i == selected_vms? 0xffffffff : alpha);
      myvertex.u = 0.0;
      myvertex.v = 0.0;
      myvertex.z = (i == selected_vms? 0.625 : 0.375);
      
      myvertex.x = SC_LEFT*sx+dx;
      myvertex.y = SC_TOP*sy+dy;
      ta_commit_list(&myvertex);
      
      myvertex.x = SC_RIGHT*sx+dx;
      myvertex.v = 48.0/64.0;
      ta_commit_list(&myvertex);
      
      myvertex.x = SC_LEFT*sx+dx;
      myvertex.y = SC_BOTTOM*sy+dy;
      myvertex.u = 1.0;
      myvertex.v = 0.0;
      ta_commit_list(&myvertex);
      
      myvertex.x = SC_RIGHT*sx+dx;
      myvertex.v = 48.0/64.0;
      myvertex.cmd |= TA_CMD_VERTEX_EOS;
      ta_commit_list(&myvertex);  

    }

  current_buffer++;
  current_buffer &= 3; 
  myvertex.z = 0.75;

  if(p == 1.0)
    for(i=0; i<4; i++)
      if(icon[i]) {
	mypoly.cmd =
	  TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_TRANSPARENT|TA_CMD_POLYGON_SUBLIST|
	  TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR|
	  TA_CMD_POLYGON_TEXTURED;
	mypoly.mode1 = TA_POLYMODE1_Z_GREATEREQUAL;
	mypoly.mode2 = TA_POLYMODE2_BLEND_SRC_ALPHA|
	  TA_POLYMODE2_BLEND_DST_INVALPHA|TA_POLYMODE2_FOG_DISABLED|
	  TA_POLYMODE2_TEXTURE_REPLACE|TA_POLYMODE2_U_SIZE_32|
	  TA_POLYMODE2_V_SIZE_32|TA_POLYMODE2_ENABLE_ALPHA|
	  TA_POLYMODE2_TEXTURE_CLAMP_U|TA_POLYMODE2_TEXTURE_CLAMP_V|
	  TA_POLYMODE2_BILINEAR_FILTER;
	mypoly.texture = TA_TEXTUREMODE_CLUT4|TA_TEXTUREMODE_CLUTBANK4(16)|
	  TA_TEXTUREMODE_TWIDDLED|TA_TEXTUREMODE_ADDRESS(icon_texture[i]);
	
	ta_commit_list(&mypoly);
	
	myvertex.cmd = TA_CMD_VERTEX;
	myvertex.colour = 0;
	myvertex.u = 0.0;
	myvertex.v = 0.0;
	
	myvertex.x = SC_LEFT+(2+i*12)*SCALE;
	myvertex.y = SC_BOTTOM+SCALE;
	ta_commit_list(&myvertex);
	
	myvertex.x += 8.0*SCALE;
	myvertex.v = 1.0;
	ta_commit_list(&myvertex);
	
	myvertex.x -= 8.0*SCALE;
	myvertex.y += 8.0*SCALE;
	myvertex.u = 1.0;
	myvertex.v = 0.0;
	ta_commit_list(&myvertex);
	
	myvertex.x += 8.0*SCALE;
	myvertex.v = 1.0;
	myvertex.cmd |= TA_CMD_VERTEX_EOS;
	ta_commit_list(&myvertex);  
      }
}

void display_progress_bar(float p)
{
  struct polygon_list mypoly;
  struct packed_colour_vertex_list myvertex;

  draw_screen(p<0.25? p*4 : 1.0);

  mypoly.cmd =
    TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_TRANSPARENT|TA_CMD_POLYGON_SUBLIST|
    TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR|
    TA_CMD_POLYGON_GOURAUD_SHADING;
  mypoly.mode1 = TA_POLYMODE1_Z_ALWAYS|TA_POLYMODE1_NO_Z_UPDATE;
  mypoly.mode2 = TA_POLYMODE2_BLEND_SRC_ALPHA|TA_POLYMODE2_BLEND_DST_INVALPHA|
    TA_POLYMODE2_FOG_DISABLED|TA_POLYMODE2_ENABLE_ALPHA;
  mypoly.texture = 0;

  ta_commit_list(&mypoly);

  myvertex.cmd = TA_CMD_VERTEX;
  myvertex.ocolour = 0;
  myvertex.z = 0.5;

  myvertex.colour = 0xc0ff0000;
  myvertex.x = 10.0;
  myvertex.y = 200.0;
  ta_commit_list(&myvertex);

  myvertex.y = 240.0;
  ta_commit_list(&myvertex);

  myvertex.x = 10.0+620.0*p;
  myvertex.y = 200.0;
  myvertex.colour = 0xc000ff00;
  ta_commit_list(&myvertex);

  myvertex.y = 240.0;
  myvertex.cmd |= TA_CMD_VERTEX_EOS;
  ta_commit_list(&myvertex);

  ta_commit_frame();
}

int openmainwin()
{
  unsigned int (*pal)[64][16] = (unsigned int (*)[64][16])0xa05f9000;
  int i, j;
  static void draw_id(int n);

  for(i=0; i<1024; i++)
    twtab[i] = (i&1)|((i&2)<<1)|((i&4)<<2)|((i&8)<<3)|((i&16)<<4)|
      ((i&32)<<5)|((i&64)<<6)|((i&128)<<7)|((i&256)<<8)|((i&512)<<9);

  dc_setup_ta();

  memset(&mainimg, 0, sizeof(mainimg));
  for(i=0; i<4+VMSCNT; i++) {
    screen_texture[i] = ta_txalloc(sizeof(mainimg));
    if(i >= 4)
      draw_id(i-4);
    texture_memcpy64(screen_texture[i], &mainimg,
		     sizeof(mainimg)/64);
    memset(&mainimg, 0, sizeof(mainimg));
  }
  current_buffer = 0;
  icon[0] = icon[1] = icon[2] = icon[3] = 0;
  for(i=0; i<4; i++)
    icon_texture[i] = mkicontexture(icon_imagery[i]);

  (*pal)[16][0] = 0 /* 0xffaad5c3*/;
  (*pal)[16][1] = 0xff081052;

  bg_texture = mkbgpixmap(vmu_large, &bg_tex_w, &bg_tex_h, &bg_tex_mode2);  

  return 1;
}

void error_msg(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);
}

void vmputpixel(int x, int y, int p)
{
  if(y<32) {
    char *s = &mainimg.c[twtab[y]+(twtab[x]>>1)];
    p&=1;
    if(x&1)
      *s = ((*s)&0xf)|(p<<4);
    else
      *s = ((*s)&~0xf)|p;
  } else if(y==38) switch(x) {
  case 8+0*12: icon[0] = p&1; break;
  case 8+1*12: icon[1] = p&1; break;
  case 8+2*12: icon[2] = p&1; break;
  case 8+3*12: icon[3] = p&1; break;
  }
}

static void draw_idchar(const unsigned char *bm, int x)
{
  int u, v, p;
  unsigned char z;
  for(u=0; u<32; u++)
    for(v=0; v<24; v+=8) {
      z = *bm++;
      for(p=0; p<8; p++, z>>=1)
	if(z&1)
	  vmputpixel(x+v+p, u, 1);
    }
}

static void draw_id(int n)
{
  draw_idchar(idLetters[n&3], 0);
  draw_idchar(idDigits[n>>2], 24);
}

void redrawlcd()
{
  draw_screen(1.0);
  ta_commit_frame();
}

void checkevents()
{
  int i=0, mask = getimask();
  int trig=0, pressed = -1;
  static int oldpressed = -1;
  static int oldtrig = 0;
  struct mapledev *pad;
  setimask(15);
  for(pad = locked_get_pads(); i<4; i++, pad++)
    if(pad->func & MAPLE_FUNC_CONTROLLER) {
      int buttons = pad->cond.controller.buttons;
      if(!(buttons & 0x060e)) exit(0);
      pressed &= ((buttons & 0xf0)>>4)|
	((buttons & 4)<<2) | ((buttons & 2)<<4) |
	((buttons & 0x600)>>3);
      if(pad->cond.controller.rtrigger > trig)
	trig = pad->cond.controller.rtrigger;
    }
  setimask(mask);
  if(trig < 0x40)
    oldtrig = 0;
  else if(trig > 0xc0 && !oldtrig) {
    oldtrig = 1;
    screen_filtering ^= TA_POLYMODE2_BILINEAR_FILTER;
  }
  if(pressed != oldpressed) {
    int i, bits = ~oldpressed & pressed;
    for(i=0; i<8; i++)
      if((bits>>i) & 1)
	keyrelease(i);
    bits = ~pressed & oldpressed;
    for(i=0; i<8; i++)
      if((bits>>i) & 1)
	keypress(i);
    oldpressed = pressed;
  }
}

void waitforevents(struct timeval *t)
{
  if(!t)
    for(;;)
      checkevents();
  else {
    unsigned long now = Timer();
    unsigned long then = now + USEC_TO_TIMER(t->tv_sec*1000000+t->tv_usec);
    while(((long)(Timer()-then))<0);
  }
}

void sleep(int n)
{
  while(n>0) {
    usleep(1000000);
    --n;
  }
}

int gettimeofday(struct timeval *tp, struct timezone *tz)
{
  static unsigned long last_tm = 0;
  static unsigned long tmhi = 0;
  unsigned long tmlo = Timer();
  if(tmlo < last_tm)
    tmhi++;

  unsigned long long usecs = 
    ((((unsigned long long)tmlo)<<11)|
     (((unsigned long long)tmhi)<<43))/100;

  tp->tv_usec = usecs % 1000000;
  tp->tv_sec = usecs / 1000000;

  last_tm = tmlo;
  return 0;
}

static void draw_real_lcd(unsigned char *dst, const unsigned char *src)
{
  int i;
  for(i=0; i<32; i++) {
    *--dst = *src++;
    *--dst = *src++;
    *--dst = *src++;
    dst -= 3;
  }
}

static void setlcdimage(struct vmsinfo *info, int i)
{
  static unsigned int tmppkt[50];

  tmppkt[0] = MAPLE_FUNC_LCD<<24;
  tmppkt[1] = 0;
  draw_real_lcd(32*6+(unsigned char *)(tmppkt+2), idLetters[i&3]);
  draw_real_lcd(32*6-3+(unsigned char *)(tmppkt+2), idDigits[i>>2]);
  maple_docmd(info->port, info->dev, MAPLE_COMMAND_BWRITE, 50, tmppkt);
}

static void check_vmsavail()
{
  int i, mask = getimask();
  struct mapledev *pad;
  struct vmsinfo info;
  struct superblock super;
  setimask(15);
  pad = locked_get_pads();
  for(i=0; i<VMSCNT; i++)
    if(pad[i&3].present & (1<<(i>>2))) {
      if(!vmsexist[i])
	vmsexist[i] = 3;
    } else
      vmsexist[i] = 0;
  setimask(mask);
  for(i=0; i<VMSCNT; i++)
    if(vmsexist[i] == 3)
      if(vmsfs_check_unit((i&3)*6+(i>>2)+1, 0, &info) &&
	 vmsfs_get_superblock(&info, &super)) {
	vmsexist[i] = 2;
	setlcdimage(&info, i);
      } else
	vmsexist[i] = 1;
}

void commit_dummy_transpoly()
{
  struct polygon_list mypoly;

  mypoly.cmd =
    TA_CMD_POLYGON|TA_CMD_POLYGON_TYPE_TRANSPARENT|TA_CMD_POLYGON_SUBLIST|
    TA_CMD_POLYGON_STRIPLENGTH_2|TA_CMD_POLYGON_PACKED_COLOUR;
  mypoly.mode1 = TA_POLYMODE1_Z_ALWAYS|TA_POLYMODE1_NO_Z_UPDATE;
  mypoly.mode2 =
    TA_POLYMODE2_BLEND_SRC_ALPHA|TA_POLYMODE2_BLEND_DST_INVALPHA|
    TA_POLYMODE2_FOG_DISABLED|TA_POLYMODE2_ENABLE_ALPHA;
  mypoly.texture = 0;
  mypoly.red = mypoly.green = mypoly.blue = mypoly.alpha = 0;
  ta_commit_list(&mypoly);
}

int selectvm()
{
  extern unsigned char sfr[0x100];
  int oldkey, wobcnt=0;
  sfr[0x4c] = 0xff;
  checkevents();
  oldkey = sfr[0x4c];
  for(;;) {
    int newkey, pressed;
    check_vmsavail();
    if(selected_vms >= 0 && vmsexist[selected_vms] != 2)
      selected_vms = -1;
    if(selected_vms < 0) {
      for(selected_vms = 0; selected_vms < VMSCNT; selected_vms++)
	if(vmsexist[selected_vms] == 2)
	  break;
      if(selected_vms >= VMSCNT)
	selected_vms = -1;
    }
    checkevents();
    newkey = sfr[0x4c];
    pressed = oldkey & ~newkey;
    oldkey = newkey;
    if(pressed & (1<<7))
      return -1;
    if(pressed & (1<<4))
      break;
    if(selected_vms >= 0) {
      if((pressed & ((1<<0)|(1<<1))) && vmsexist[selected_vms^4]==2)
	selected_vms ^= 4;
      if((pressed & ((1<<2)|(1<<3)))) {
	int dx = ((pressed & (1<<3))? 1:-1);
	int y = selected_vms & 4;
	int x = selected_vms & 3;
	int i;
	for(i=0; i<4; i++) {
	  x = (x+dx)&3;
	  if(vmsexist[x|y] == 2) {
	    selected_vms = x|y;
	    break;
	  } else if(vmsexist[x|y^4] == 2) {
	    selected_vms = x|y^4;
	    break;
	  }
	}
      }
    }
    {
      float ss, cc;
      wobcnt = ++wobcnt & 63;
      SINCOS(wobcnt<<10, ss, cc);
      wobble = 1.0+0.125*ss;
    }
    draw_screen(0.0);
    commit_dummy_transpoly();
    ta_commit_frame();
  }

  return selected_vms;
}

void unselectvm()
{
  int i;
  check_vmsavail();
  for(i=50; i>=0; --i) {
    draw_screen(i*(1.0/50));
    ta_commit_frame();
  }
  memset(&mainimg, 0, sizeof(mainimg));
  icon[0] = icon[1] = icon[2] = icon[3] = 0;
}

void sound(int freq)
{
  static int sound_inited = 0;
  if(!sound_inited) {
    stop_sound();
    do_sound_command(CMD_SET_FREQ_EXP(FREQ_22050_EXP));
    do_sound_command(CMD_SET_STEREO(0));
    do_sound_command(CMD_SET_BUFFER(3));
    sound_inited = 1;
  }
  int ringsamp = read_sound_int(&SOUNDSTATUS->ring_length)+10;
  if(freq<0)
    memset4(RING_BUF, 0, SAMPLES_TO_BYTES(ringsamp));
  else {
    int i;
    short *samps = alloca(SAMPLES_TO_BYTES(ringsamp));
    static short v = 0x7fff;
    static int f = 0;
    for(i=0; i<ringsamp; i++) {
      f += freq;
      while(f >= 22050) {
	v ^= 0xffff;
	f -= 22050;
      }
      samps[i] = v;
    }
    memcpy4(RING_BUF, samps, SAMPLES_TO_BYTES(ringsamp));
    start_sound();
  }
}
