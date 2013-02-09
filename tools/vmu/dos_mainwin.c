#include "allegro.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include "prototypes.h"
#include "vmu.xpm"
#include "vmu.5.xpm"

extern int gDebugMode,gFGColor,gBGColor;
BITMAP *back_bm=NULL;
PALETTE *back_pal=NULL;

int pixdbl = 3;
int bm_x,bm_y,bm_w,bm_h;
int off_x,off_y;

void error_msg(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  fputc('\n', stderr);
  va_end(va);
}

BITMAP *xpmtobitmap(char **vmu1_xpm,PALETTE **pret)
{
  	int i, w, h, nc, x, y;
  	PALETTE *p;
	BITMAP *bret;

	p=(PALLETE *)malloc(sizeof(PALETTE));
	if(!p)
	{			
		*pret=NULL;
		return(NULL);					  
	}
	memset(p,0,sizeof(PALETTE));

  	sscanf(vmu1_xpm[0], "%d %d %d", &w, &h, &nc);
	bret=create_bitmap(w,h);
	if(!bret)
	{
		free(p);
		*pret=NULL;
		return(NULL);
	}					  
	clear(bret);

  	for(i=1; i<=nc; i++)
	{
   	int r, g, b;
   	sscanf(vmu1_xpm[i]+4, "#%2x%2x%2x", &r, &g, &b);
		(*p)[vmu1_xpm[i][0]].r=r>>2;
		(*p)[vmu1_xpm[i][0]].g=g>>2;
		(*p)[vmu1_xpm[i][0]].b=b>>2;
  	}
  	for(y=0; y<h; y++)
	{
		unsigned char *off=(unsigned char *)vmu1_xpm[i++];
		for(x=0; x<w; x++)
			putpixel(bret,x,y,*off++);
  	}
	*pret=p;
	return bret;
}

void fakebackground()
{
	BITMAP *bm;
	PALETTE *pal;
			  
	bm=xpmtobitmap(vmu_large,&pal);
	set_palette(*pal);
	clear(screen);
	blit(bm,screen,0,0,(SCREEN_W-bm->w)>>1,(SCREEN_H-bm->h)>>1,bm->w,bm->h);
	gFGColor='C';
	gBGColor='a';
}

BITMAP *genbackbitmap(PALETTE **pret)
{
	BITMAP *bm,*tmp;
	PALETTE *pal;
	int neww,newh;
			  
	switch(pixdbl)
	{
	case 0:		                         /*CC- tiny 48x32 display */
		bm=xpmtobitmap(vmu_small,&pal);
		bm_w=bm->w;				
		bm_h=bm->h;				
		bm_x=(SCREEN_W-bm->w)>>1;				
		bm_y=(SCREEN_H-bm->h)>>1;				
		off_x=bm_x+18;
		off_y=bm_y+43;
		gFGColor='C';
		gBGColor='a';
		break;
	case 1:                              /*CC- small lcd 96x64 display */
		bm=xpmtobitmap(vmu_large,&pal);
		bm_w=bm->w;				
		bm_h=bm->h;				
		bm_x=(SCREEN_W-bm->w)>>1;				
		bm_y=(SCREEN_H-bm->h)>>1;				
		off_x=bm_x+32;
		off_y=bm_y+85;
		gFGColor='C';
		gBGColor='a';
		break;
	case 2:		                         /*CC- small full 96x64 display */
		bm=xpmtobitmap(vmu_large,&pal);
		bm_w=bm->w;				
		bm_h=bm->h;				
		bm_x=(SCREEN_W-bm->w)>>1;				
		bm_y=(SCREEN_H-bm->h)>>1;				
		off_x=bm_x+32;
		off_y=bm_y+85;
		gFGColor='C';
		gBGColor='a';
		break;
	case 3:                              /*CC- large lcd 144x80 display */
		bm=xpmtobitmap(vmu_large,&pal);
		neww=bm->w+(bm->w>>1);
		newh=bm->h+(bm->h>>1);
		tmp=create_bitmap(neww,newh);
		stretch_blit(bm,tmp,0,0,bm->w,bm->h,0,0,neww,newh);
		destroy_bitmap(bm);
		bm=tmp;
		bm_w=bm->w;				
		bm_h=bm->h;				
		bm_x=(SCREEN_W-bm->w)>>1;				
		bm_y=((SCREEN_H-bm->h)>>1)+20;    /*CC- special hack so we can see DC logo! */
		off_x=bm_x+50;
		off_y=bm_y+130;
		gFGColor='C';
		gBGColor='a';
		break;
	}
	*pret=pal;
	return(bm);
}
			  
int screeninited=0;

void vmputpixel(int x, int y, int p)
{
	int i;
   char s[256];
   int c;

   if(!screeninited)
	{
		if(!back_bm)
			back_bm=genbackbitmap(&back_pal);
      clear(screen);
      /* blit the image onto the screen */
		if(back_bm)
		{
			set_palette(*back_pal);		  
         blit(back_bm,screen,0,0,bm_x,bm_y,bm_w,bm_h);
		}			
		else
			printf("NO BACKGROUND\n");
		screeninited=1;
	}
   c=(p&1)?gFGColor:gBGColor;
   switch(pixdbl)
   {
   case 0:      
      putpixel(screen,off_x+x     ,off_y+y     ,c);
      break;
   case 1:      
      putpixel(screen,off_x+(x<<1),off_y+(y<<1),c);
      break;
   case 2:      
      putpixel(screen,off_x+(x<<1)  ,off_y+(y<<1)  ,c);
      putpixel(screen,off_x+(x<<1)+1,off_y+(y<<1)  ,c);
      putpixel(screen,off_x+(x<<1)  ,off_y+(y<<1)+1,c);
      putpixel(screen,off_x+(x<<1)+1,off_y+(y<<1)+1,c);
      break;
   case 3:      
      putpixel(screen,off_x+(x*3)  ,off_y+(y*3)  ,c);
      putpixel(screen,off_x+(x*3)+1,off_y+(y*3)  ,c);
      putpixel(screen,off_x+(x*3)  ,off_y+(y*3)+1,c);
      putpixel(screen,off_x+(x*3)+1,off_y+(y*3)+1,c);
      break;
   }
}

void low_redrawlcd()
{
   static int frame=0;

   if(gDebugMode)
      textprintf_centre(screen, font, SCREEN_W/2, 20, gFGColor,"Frame %d",frame++);
}

void redrawlcd()
{
  low_redrawlcd();
}

void checkevents()
{
   static old_up=0;
   static old_dn=0;
   static old_rt=0;
   static old_lt=0;
   static old_a=0;
   static old_b=0;
   static old_m=0;
   static old_s=0;
   char s[256];

   if(key[KEY_UP]   &&!old_up) {      
         old_up=1;
		   keypress(0);
   }
   if(key[KEY_DOWN] &&!old_dn) {      
         old_dn=1;
         keypress(1);
   }
   if(key[KEY_LEFT] &&!old_lt) {      
         old_lt=1;
         keypress(2);
   }
   if(key[KEY_RIGHT]&&!old_rt) {      
         old_rt=1;
         keypress(3);
   }
   if(key[KEY_Z]    &&!old_a) {      
         old_a=1;
         keypress(4);
   }
   if(key[KEY_X]    &&!old_b) {      
         old_b=1;
         keypress(5);
   }
   if(key[KEY_S]    &&!old_s) {      
         old_s=1;
         keypress(6);
   }
   if(key[KEY_M]    &&!old_m) {      
         old_m=1;
         keypress(7);
   }
   if(key[KEY_ESC])
      exit(0);

   if(!key[KEY_UP]   &&old_up) {      
         old_up=0;
         keyrelease(0);
   }
   if(!key[KEY_DOWN] &&old_dn) {      
         old_dn=0;
         keyrelease(1);
   }
   if(!key[KEY_LEFT] &&old_lt) {      
         old_lt=0;
         keyrelease(2);
   }
   if(!key[KEY_RIGHT]&&old_rt) {      
         old_rt=0;
         keyrelease(3);
   }
   if(!key[KEY_Z]    &&old_a) {      
         old_a=0;
         keyrelease(4);
   }
   if(!key[KEY_X]    &&old_b) {      
         old_b=0;
         keyrelease(5);
   }
   if(!key[KEY_S]    &&old_s) {      
         old_s=0;
         keyrelease(6);
   }
   if(!key[KEY_M]    &&old_m) {      
         old_m=0;
         keyrelease(7);
   }
   if(gDebugMode)
      textprintf_centre(screen, font,SCREEN_W/2, 0, gFGColor,"KEYS: %d%d%d%d%d%d%d%d",old_up,old_dn,old_lt,old_rt,old_a,old_b,old_s,old_m);
}

void waitforevents(struct timeval *t)
{
   struct timeval now,dest;

   if(!t)
      return;
   gettimeofday(&dest, NULL);
   dest.tv_sec+=t->tv_sec;
   dest.tv_usec+=t->tv_usec;
   while(dest.tv_usec>1000000)
   {
      dest.tv_sec++;
      dest.tv_usec-=1000000;
   }         
   gettimeofday(&now, NULL);
//   printf("now=%d:%d\ndest=%d:%d\n",now.tv_sec,now.tv_usec,dest.tv_sec,dest.tv_usec);
   while(1)
   {
      gettimeofday(&now, NULL);
      if(now.tv_sec>dest.tv_sec)
         return;
      if(now.tv_sec==dest.tv_sec && now.tv_usec>=dest.tv_usec)
         return;
   }      
}

void sound(int freq)
{
}
