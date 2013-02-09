#include "allegro.h"
#include <stdarg.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "version.h"
#include "prototypes.h"

int gDebugMode=0,gFGColor,gBGColor;
extern BITMAP *back_bm;
extern PALETTE *back_pal;
extern char *vmu_large[];
int gForceSpeed=0;
extern void fakebackground();

extern int pixdbl;
extern int optind;
extern char *optarg;
extern int getopt(int argc, char * const * argv, const char *optstring);
 
int def_fg,def_bg,img_fg,img_bg;

int prepare(char *msg,...)
{      
	va_list va;
	va_start(va,msg);

  	vprintf(msg,va);
   printf("Press any to key to start\n");
   readkey();
   set_gfx_mode(GFX_VGA, 320, 200, 0, 0);
	va_end(va);
}

int main(int argc, char *argv[])
{
  int c;
  char *bios = NULL;

  def_fg=255;
  def_bg=0;
  img_fg=13;
  img_bg=132;

   while((c=getopt(argc, argv, ":dhV0123s:b:"))!=EOF)
   {
      switch(c)
      {
      case 'h':
         fprintf(stderr, "Usage: %s [-h][-d][-V][-0][-1][-2][-3][-sSpeed][-IFILE.PCX][-BBgCol][-FFgCol][-b bios] game.vms\n"
                         "       -h      Displays this info\n"
                         "       -d      Debug mode\n"
                         "       -V      Display version number\n"
                         "       -0      Single pixel display mode (small)\n"
                         "       -1      Single pixel display mode (medium)\n"
                         "       -2      Double pixel display mode (medium)\n"
                         "       -3      Triple pixel display mode (large) - default\n"
                         "       -s:NNN  Change throttle speed to NNN (numeric)\n"
                         "       -b bios Load VMS BIOS from this file (64K)\n",argv[0]);
         break;
      case 's':
         gForceSpeed=atoi(optarg+1);
         fprintf(stderr, "Speed forced to %d\n",gForceSpeed);
         break;
      case 'V':
         fprintf(stderr, "softvms " VERSION " by Marcus Comstedt <marcus@idonex.se> and Colm Cox <ccox@elandtech.com>\n");
         break;
      case '0':
         pixdbl = 0;
         break;
      case '1':
         pixdbl = 1;
         break;
      case '2':
         pixdbl = 2;
         break;
      case '3':
         pixdbl = 3;
         break;
      case 'd':
         gDebugMode=1;
         break;
       case 'b':
	 		bios = optarg;
	 		break;
      }
   }
   allegro_init();
   install_keyboard(); 
   text_mode(0);

   if(argc>optind)
   {
      int fd = open(argv[optind], O_RDONLY|O_BINARY);
      int r;
      char hdr[4];
      if(fd<0 || (r=read(fd, hdr, 4)<4))
      {
         perror(argv[optind]);
         if(fd>=0)
            close(fd);
         clear_keybuf();
         return 1;
      }

      if(hdr[0]=='L' && hdr[1]=='C' && hdr[2]=='D' && hdr[3]=='i')
      {     
         prepare("Starting with lcdimage file:%s bios:%s\n",argv[optind],bios?bios:"(none)");
         do_lcdimg(argv[optind]);
      }
      else
      {      
         prepare("Starting with vmsgame:%s bios:%s\n",argv[optind],bios?bios:"(none)");
         do_vmsgame(argv[optind], bios);
      }
   }
   else if(bios != NULL)
   {
     prepare("Starting with bios:%s\n",bios);
     do_vmsgame(NULL, bios);
   }
   else
   {      
      prepare("No game/lcdimage/bios specified.\n",NULL);
		fakebackground();
	   text_mode(-1);
      textprintf_centre(screen, font,SCREEN_W/2, 70, gFGColor, "NO GAME");
      textprintf_centre(screen, font,SCREEN_W/2, 80, gFGColor, "LOADED.");
      textprintf_centre(screen, font,SCREEN_W/2, 100, gFGColor, "HIT ESC");
      textprintf_centre(screen, font,SCREEN_W/2, 110, gFGColor, "TO EXIT");
		for(;;)
      {
         waitforevents(NULL);
         checkevents();
      }
   }
   clear_keybuf();
   return 0;
}


