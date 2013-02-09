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

#define SGNEXT(n) ((n)&0x80? (n)-0x100:(n))

#ifndef BIG
#define BIG 
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

BIG unsigned char flash[0x20000];
BIG unsigned char bios[0x10000];
unsigned char ram[2][0x100];
unsigned char sfr[0x100];
unsigned char xram[3][0x80];
unsigned char wram[0x200];
unsigned char *rom;

unsigned char parity[0x100];

int pc;
int lcd_updated, lcdon, imask, intreq, hasbios=0;
int spd;
int t0h, t0l, t0base, t0scale;
int t1h, t1l;
int gamesize;

void keypress(int i)
{
  sfr[0x4c]&=~(1<<i);
  if(sfr[0x4e]&4)
    sfr[0x4e]|=2;
}

void keyrelease(int i)
{
  sfr[0x4c]|=(1<<i);
  if(sfr[0x4e]&4)
    sfr[0x4e]|=2;
}

int tobcd(int n)
{
  return ((n/10)<<4)|(n%10);
}

int initflash(int fd)
{
  int r=0, t=0;
  memset(flash, 0, sizeof(flash));
  while(t<sizeof(flash) && (r=read(fd, flash+t, sizeof(flash)-t))>0)
    t += r;
  if(r<0 || t<0x480)
    return 0;
  return t;
}

void fakeflash(char *filename, int sz)
{
  unsigned char *root, *fat, *dir;
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  int i;
  char *fn2;

  if((fn2 = strrchr(filename, '/'))!=NULL)
    filename = fn2+1;
  else if((fn2 = strrchr(filename, '\\'))!=NULL)
    filename = fn2+1;
  memset(flash+241*512, 0, sizeof(flash)-241*512);
  root = flash+255*512;
  fat = flash+254*512;
  dir = flash+253*512;
  sz = ((sz+511)>>9);
  for(i=0; i<256*2; i+=2) {
    fat[i] = 0xfc;
    fat[i+1] = 0xff;
  }
  for(i=0; i<sz; i++) {
    fat[2*i] = i+1;
    fat[2*i+1] = 0;
  }
  if((--i)>=0) {
    fat[2*i] = 0xfa;
    fat[2*i+1] = 0xff;
  }
  fat[254*2] = 0xfa;
  fat[254*2+1] = 0xff;
  fat[255*2] = 0xfa;
  fat[255*2+1] = 0xff;
  for(i=253; i>241; --i) {
    fat[2*i] = i-1;
    fat[2*i+1] = 0;
  }
  fat[241*2] = 0xfa;
  fat[241*2+1] = 0xff;
  dir[0] = 0xcc;
  strncpy(dir+4, filename, 12);
  for(i=strlen(filename); i<12; i++)
    dir[4+i]=' ';
  dir[0x10] = tobcd(tm->tm_year/100+19);
  dir[0x11] = tobcd(tm->tm_year%100);
  dir[0x12] = tobcd(tm->tm_mon+1);
  dir[0x13] = tobcd(tm->tm_mday);
  dir[0x14] = tobcd(tm->tm_hour);
  dir[0x15] = tobcd(tm->tm_min);
  dir[0x16] = tobcd(tm->tm_sec);
  dir[0x17] = tobcd(tm->tm_wday);
  dir[0x18] = sz&0xff;
  dir[0x19] = sz>>8;
  dir[0x1a] = 1;
  memset(root, 0x55, 16);
  root[0x10] = 1;
  memcpy(root+0x30, dir+0x10, 8);
  root[0x44] = 255;
  root[0x46] = 254;
  root[0x48] = 1;
  root[0x4a] = 253;
  root[0x4c] = 13;
  root[0x50] = 200;
}

void check_gamesize()
{
  unsigned char *root, *fat, *dir;
  int i, fatblk, dirblk, dircnt;
  root = flash+255*512;
  fatblk = (root[0x47]<<8)|root[0x46];
  dirblk = (root[0x4b]<<8)|root[0x4a];
  dircnt = (root[0x4d]<<8)|root[0x4c];
  if(fatblk>=256 || dircnt>=256 || !dircnt)
    return;
  fat = flash+fatblk*512;
  while(dircnt-- && dirblk<256) {
    dir = flash+dirblk*512;
    dirblk = (fat[2*dirblk+1]<<8)|fat[2*dirblk];
    for(i=0; i<16; i++)
      if(dir[i*32] == 0xcc) {
	int sz = (dir[i*32+0x19]<<8)|dir[i*32+0x18];
	if(sz>1 && sz<=200) {
	  gamesize = sz*512;
	  return;
	}
      }
  }
}

int loadflash(char *filename)
{
  int r, fd;
  
  if((fd = open(filename, O_RDONLY|O_BINARY))>=0) {
    if(!(r=initflash(fd))) {
      close(fd);
      error_msg("%s: can't load FLASH image", filename);
      return 0;
    }
    close(fd);
    gamesize = r;
    if(r<241*512)
      fakeflash(filename, r);
    check_gamesize();
    return 1;
  } else {
    perror(filename);
    return 0;
  }
}

int initbios(int fd)
{
  int r=0, t=0;
  memset(bios, 0, sizeof(bios));
  while(t<sizeof(bios) && (r=read(fd, bios+t, sizeof(bios)-t))>0)
    t += r;
  if(r<0 || t<0x200)
    return 0;
  return 1;
}

int loadbios(char *filename)
{
  int fd;
  
  if((fd = open(filename, O_RDONLY|O_BINARY))>=0) {
    if(!initbios(fd)) {
      close(fd);
      error_msg("%s: can't load BIOS image", filename);
      return 0;
    }
    close(fd);
    hasbios=1;
    return 1;
  } else {
    perror(filename);
    return 0;
  }
}

void halt_mode()
{
  waitforevents(NULL);
  checkevents();
}

void lcdrefresh()
{
  static unsigned char icon[4][7] = {
    { 0x2a, 0x23, 0x47, 0xfb, 0x23, 0x23, 0x3f },
    { 0xff, 0x83, 0x93, 0xbb, 0x93, 0xbb, 0xff },
    { 0x7c, 0x82, 0x92, 0x9b, 0x82, 0x82, 0x7c },
    { 0x10, 0x28, 0xef, 0x6e, 0x6c, 0x7e, 0xc3 }
  };
  int y, x, i, a, b=0, p=0;

  p = sfr[0x22];
  if(p>=0x83)
    p -= 0x83;
  b = (p>>6);
  p = (p&0x3f)*2;
  for(y=0; y<32; y++) {
    for(x=0; x<48; ) {
      int value = xram[b][p++];
      if(!lcdon)
	value = 0;
      vmputpixel(x++, y, value>>7);
      vmputpixel(x++, y, value>>6);
      vmputpixel(x++, y, value>>5);
      vmputpixel(x++, y, value>>4);
      vmputpixel(x++, y, value>>3);
      vmputpixel(x++, y, value>>2);
      vmputpixel(x++, y, value>>1);
      vmputpixel(x++, y, value);
      if((p&0xf)>=12)
	p+=4;
      if(p>=128) {
	b++;
	p-=128;
      }
      if(b==2 && p>=6) {
	b = 0;
	p -= 6;
      }
    }
  }
  p++;
  if((p&0xf)>=12)
    p+=4;
  if(p>=128) {
    b++;
    p-=128;
  }
  if(b==2 && p>=6) {
    b = 0;
    p -= 6;
  }
  for(i=0; i<4; i++) {
    a = xram[b][p++]>>(2*(3-i));
    if(!lcdon)
      a = 0;
    for(y=0; y<7; y++) {
      int value = icon[i][y];
      for(x=0; x<7; x++)
	vmputpixel(2+i*12+x, y+33, (value>>(7-x))&a);
    }
    if((p&0xf)>=12)
      p+=4;
    if(p>=128) {
      b++;
      p-=128;
    }
    if(b==2 && p>=6) {
      b = 0;
      p -= 6;
    }
  }
  redrawlcd();
  lcd_updated = 0;
}

void writemem(int addr, int value)
{
  value &= 0xff;
  if(addr<0x100) {
    ram[(sfr[0x01]&2)>>1][addr] = value;
    return;
  }
  if(addr>=0x180) {
    int b = sfr[0x25];
    if(b>2 || (b==2 && addr>=0x186))
      return;
    xram[b][addr-0x180]=value;
    if(lcdon)
      lcd_updated = 1;
  } else switch(addr) {
   case 0x100:
     sfr[0x01] = (sfr[0x01]&0xfe)|parity[value];
     break;
   case 0x10d:
     if((value&1) != (sfr[0x0d]&1)) {
       if(pc>0xfffd || rom[pc]!=0x21)
	 error_msg("EXT 0 changed without following JMPF.  pc = %04x",
		   pc&0xffff);
       else
	 pc = (rom[pc+1]<<8)|rom[pc+2];
       if(hasbios)
	 rom = ((value&1)? flash : bios);
     }
     break;
   case 0x10e:
     switch(value&0xa0) {
      case 0x00: spd = 3000; break;
      case 0x20: spd = 164; break;
      case 0x80: spd = 6000; break;
      case 0xa0: spd = 328; break;
     }
     break;
   case 0x110:
     if(!(value&0x40))
       t0l = sfr[0x13];
     if(!(value&0x80))
       t0h = sfr[0x15];
     break;
   case 0x111:
     t0scale = 256-value;
     t0base = 0;
     break;
   case 0x113:
     if(!(sfr[0x10]&0x40))
       t0l = value;
     break;
   case 0x115:
     if(!(sfr[0x10]&0x80))
       t0h = value;
     break;
   case 0x118:
     if(!(value&0x40))
       t1l = sfr[0x1b];
     if(!(value&0x80))
       t1h = sfr[0x1d];
     break;
   case 0x11b:
     if(!(sfr[0x18]&0x40))
       t1l = value;
     break;
   case 0x11d:
     if(!(sfr[0x18]&0x80))
       t1h = value;
     break;
   case 0x122:
     if(lcdon)
       lcd_updated = 1;
     break;
   case 0x127:
     if((!!(value&0x80)) != lcdon) {
       lcdon = !!(value&0x80);
       lcdrefresh();
     }
     break;
   case 0x166:
     wram[0x1ff&((sfr[0x65]<<8)|sfr[0x64])] = value;
     if(sfr[0x63]&0x10)
       if(!++sfr[0x64])
	 sfr[0x65]^=1;
     return;
  }
  /*
    if(addr>0x10e && addr<0x120 && addr != 0x118)
      fprintf(stderr, "%04x: Write to %03x: %02x\n", pc, addr, value);
  */
  sfr[addr&0xff] = value;
  if(addr == 0x118 || addr == 0x11b) {
    /* Check for sound... */
    if(sfr[0x18]&0x40)
      sound(32768/((256-sfr[0x1b])*6));
    else
      sound(-1);
  }
}

int readmem(int addr)
{
  int r;
  if(addr<0x100)
    return ram[(sfr[0x01]&2)>>1][addr];
  if(addr>=0x180) {
    int b = sfr[0x25];
    if(b>2)
      return 0xff;
    return xram[b][addr-0x180];
  } else switch(addr) {
   case 0x112:
     return t0l;
   case 0x114:
     return t0h;
   case 0x11b:
     return t1l;
   case 0x11d:
     return t1h;
   case 0x15c:
     return 2;
   case 0x165:
     return 0xfe|(sfr[0x65]&1);
   case 0x166:
     r = wram[0x1ff&((sfr[0x65]<<8)|sfr[0x64])];
     if(sfr[0x63]&0x10)
       if(!++sfr[0x64])
	 sfr[0x65]^=1;
     return r;
  }
  /*
  if(addr>0x106 && addr<0x180)
    fprintf(stderr, "%04x: Read from %03x: %02x\n", pc, addr, sfr[addr&0xff]);
  */
  return sfr[addr&0xff];
}

int readlatch(int addr)
{
  switch(addr) {
   case 0x11b:
   case 0x11d:
     return 0xff;
   default:
     return readmem(addr);
  }
}

void push(int n)
{
  writemem(0x106, readmem(0x106)+1);
  ram[0][readmem(0x106)]=n;
}

int pop()
{
  int r = ram[0][readmem(0x106)];
  writemem(0x106, readmem(0x106)-1);
  return r;
}

void resetcpu()
{
  int i;
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  memset(ram, 0, sizeof(ram));
  memset(xram, 0, sizeof(xram));
  memset(wram, 0, sizeof(wram));
  memset(sfr, 0, sizeof(sfr));
  parity[0] = 0;
  parity[1] = 1;
  parity[2] = 1;
  parity[3] = 0;
  for(i=4; i<16; i++)
    parity[i] = parity[i&3]^parity[i>>2];
  for(i=16; i<255; i++)
    parity[i] = parity[i&15]^parity[i>>4];  
  sfr[0x06] = 0x7f;
  sfr[0x4c] = 0xff;
  sfr[0x01] = 0x02;
  t0h = t0l = 0;
  t1h = t1l = 0;
  t0base = 0;
  t0scale = 256;
  ram[0][0x10] = tobcd(tm->tm_year/100+19);
  ram[0][0x11] = tobcd(tm->tm_year%100);
  ram[0][0x12] = tobcd(tm->tm_mon+1);
  ram[0][0x13] = tobcd(tm->tm_mday);
  ram[0][0x14] = tobcd(tm->tm_hour);
  ram[0][0x15] = tobcd(tm->tm_min);
  ram[0][0x17] = (tm->tm_year+1900)>>8;
  ram[0][0x18] = (tm->tm_year+1900)&0xff;
  ram[0][0x19] = tm->tm_mon+1;
  ram[0][0x1a] = tm->tm_mday;
  ram[0][0x1b] = tm->tm_hour;
  ram[0][0x1c] = tm->tm_min;
  ram[0][0x1d] = tm->tm_sec;
  ram[0][0x31] = 0xff;
  sfr[0x08] = 0x80;
  writemem(0x10e, 0x81);
  lcd_updated = 0;
  lcdon = 0;
  imask = 0;
  intreq = 0;
  if(hasbios) {
    sfr[0x0d] = 0;
    rom = bios;
    pc = 0x1f0;
  } else {
    sfr[0x0d] = 1;
    rom = flash;
    pc = 0;
    writemem(0x125, 2);
    writemem(0x182, 0x10);
    writemem(0x125, 0);
    writemem(0x127, 0x80);
  }
  sound(-1);
}

int month_days()
{
  int m = ram[0][0x19];
  if(m==2) {
    int y = ram[0][0x18] | (ram[0][0x17] << 8);
    if(y&3)
      return 28;
    if(!(y%4000))
      return 29;
    if(!(y%1000))
      return 28;
    if(!(y%400))
      return 29;
    if(!(y%100))
      return 28;
    return 29;
  } else return (m>7? ((m&1)? 30:31) : ((m&1)? 31:30));
}

int handle_fwcall(int pc)
{
  switch(pc) {
   case 0x100:
     {
       int i, a = ((ram[1][0x7d]<<16)|(ram[1][0x7e]<<8)|ram[1][0x7f])&0x1ffff;
       if(a>=gamesize)
	 writemem(0x100, 0xff);
       else {
	 writemem(0x100, 0x00);
	 for(i=0; i<0x80; i++)
	   flash[(a&~0xff)|((a+i)&0xff)] = ram[1][i+0x80];
#ifdef __DC__
	 if(!flash_written(a))
	   writemem(0x100, 0xff);
#endif
       }
       /*
       fprintf(stderr, "ROM write @ %05x:\n", a);
       for(i=0; i<0x80; i++)
	 fprintf(stderr, " %02x", ram[1][i+0x80]);
       fprintf(stderr, "\n");
       */
     }
     return 0x105;
   case 0x110:
     {
       int i, a = ((ram[1][0x7d]<<16)|(ram[1][0x7e]<<8)|ram[1][0x7f])&0x1ffff;
       int r = 0;
       for(i=0; i<0x80; i++)
	 if((r = (flash[(a&~0xff)|((a+i)&0xff)] ^ ram[1][i+0x80])) != 0)
	   break;
       writemem(0x100, r);
     }
     return 0x115;
   case 0x120:
     {
       int i, a = ((ram[1][0x7d]<<16)|(ram[1][0x7e]<<8)|ram[1][0x7f])&0x1ffff;
       for(i=0; i<0x80; i++)
	 ram[1][i+0x80] = flash[(a&~0xff)|((a+i)&0xff)];
       /*
       fprintf(stderr, "ROM read @ %05x\n", a);
       */
     }
     return 0x125;
   case 0x130:
     if(!((ram[0][0x1e]^=1)&1))
       if(++ram[0][0x1d]>=60) {
	 ram[0][0x1d] = 0;
	 if(++ram[0][0x1c]>=60) {
	   ram[0][0x1c] = 0;
	   if(++ram[0][0x1b]>=24) {
	     ram[0][0x1b] = 0;
	     if(++ram[0][0x1a]>month_days()) {
	       ram[0][0x1a] = 1;
	       if(++ram[0][0x19]>=13) {
		 ram[0][0x19] = 1;
		 if(ram[0][0x18]==0xff) {
		   ram[0][0x18]=0;
		   ram[0][0x17]++;
		 } else
		   ram[0][0x18]++;
	       }
	     }
	   }
	 }
       }
     return 0x139;
   case 0x1f0:
     return 0;
   default:
     error_msg("Firmware entered at unknown vector %04x!", pc);
     return 0;
  }
}

void run_cpu()
{
  struct timeval epoch;
  int mcy = 0, tick = 0;

  GETTIMEOFDAY(&epoch);
  for(;;) {
    int r, s, c, cy = 1, i = rom[pc];
#ifdef TRACE
    printf("%04x\n", pc);
#endif
    pc++;
    pc &= 0xffff;
    switch(i&0xf) {
     case 0:
       switch(i>>4) {
	case 0:
	  break;
	case 1:
	  cy = 4;
	  push((pc+2)&0xff);
	  push(((pc+2)&0xff00)>>8);
	  pc = 0xffff&(pc+1+rom[pc]+((rom[(pc+1)&0xffff])<<8));
	  break;
	case 2:
	  cy = 2;
	  push((pc+2)&0xff);
	  push(((pc+2)&0xff00)>>8);
	  pc = (rom[pc]<<8)|rom[(pc+1)&0xffff];
	  break;
	case 3:
	  cy = 7;
	  r = readmem(0x102)*((readmem(0x100)<<8)|readmem(0x103));
	  writemem(0x101, (readmem(0x101)&0x7b)|(r>65535? 4:0));
	  writemem(0x103, r&0xff);
	  writemem(0x100, (r&0xff00)>>8);
	  writemem(0x102, (r&0xff0000)>>16);
	  break;
	case 4:
	  cy = 7;
	  r = readmem(0x102);
	  if(r) {
	    int v = (readmem(0x100)<<8)|readmem(0x103);
	    s = v%r;
	    r = v/r;
	  } else {
	    r = 0xff00|readmem(0x103);
	    s = 0;
	  }
	  writemem(0x101, (readmem(0x101)&0x7b)|(s? 0:4));
	  writemem(0x103, r&0xff);
	  writemem(0x100, (r&0xff00)>>8);
	  writemem(0x102, s);
	  break;
	case 5:
	  cy = 2; /* ? */
	  writemem(0x100, flash[0x1ffff&(readmem(0x104)+(readmem(0x105)<<8)+
					 (readmem(0x154)<<16))]);
	  break;
	case 6:
	  cy = 2;
	  push(readmem(rom[pc++]));
	  pc &= 0xffff;
	  break;
	case 7:
	  cy = 2;
	  writemem(rom[pc++], pop());
	  pc &= 0xffff;
	  break;
	case 8:
	  cy = 2;
	  if(readmem(0x100)==0)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 9:
	  cy = 2;
	  if(readmem(0x100)!=0)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 0xa:
	  cy = 2;
	  r = pop()<<8;
	  r |= pop();
	  pc = r;
	  break;
	case 0xb:
	  cy = 2;
	  r = pop()<<8;
	  r |= pop();
	  pc = r;
	  --imask;
	  break;
	case 0xc:
	  r = readmem(0x100);
	  writemem(0x100, (r>>1)|((r&1)<<7));
	  break;
	case 0xd:
	  r = readmem(0x100);
	  s = readmem(0x101);
	  writemem(0x101, (s&0x7f)|((r&1)<<7));
	  writemem(0x100, (r>>1)|(s&0x80));
	  break;
	case 0xe:
	  r = readmem(0x100);
	  writemem(0x100, (r<<1)|((r&0x80)>>7));
	  break;
	case 0xf:
	  r = readmem(0x100);
	  s = readmem(0x101);
	  writemem(0x101, (s&0x7f)|(r&0x80));
	  writemem(0x100, (r<<1)|((s&0x80)>>7));
	  break;
       }
       break;
     case 1:
       switch(i>>4) {
	case 0:
	  cy = 2;
	  pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  break;
	case 1:
	  cy = 4;
	  pc = 0xffff&(pc+1+rom[pc]+((rom[(pc+1)&0xffff])<<8));
	  break;
	case 2:
	  cy = 2;
	  pc = (rom[pc]<<8)|rom[(pc+1)&0xffff];
	  break;
	case 3:
	  cy = 2;
	  r = readmem(0x100);
	  writemem(0x101, (readmem(0x101)&0x7f)|(r<rom[pc]? 0x80:0));
	  s = (r == rom[pc++]);
	  pc &= 0xffff;
	  if(s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 4:
	  cy = 2;
	  r = readmem(0x100);
	  writemem(0x101, (readmem(0x101)&0x7f)|(r<rom[pc]? 0x80:0));
	  s = (r != rom[pc++]);
	  pc &= 0xffff;
	  if(s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 5:
	  cy = 2; /* ? */
	  if(!(readmem(0x154)&2))
	    flash[0x1ffff&(readmem(0x104)+(readmem(0x105)<<8)+
			   (readmem(0x154)<<16))] = readmem(0x100);
	  break;
	case 6:
	  cy = 2;
	  push(readmem(0x100|rom[pc++]));
	  pc &= 0xffff;
	  break;
	case 7:
	  cy = 2;
	  writemem(0x100|rom[pc++], pop());
	  pc &= 0xffff;
	  break;
	case 8:
	  r = readmem(0x100);
	  s = rom[pc++];
	  pc &= 0xffff;
	  writemem(0x100, r+s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s>255? 0x80:0)|
		   ((r&15)+(s&15)>15? 0x40:0)|((0x80&(~r^s)&(s^(r+s)))? 4:0));
	  break;
	case 9:
	  r = readmem(0x100);
	  s = rom[pc++];
	  pc &= 0xffff;
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r+s+c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s+c>255? 0x80:0)|
		   ((r&15)+(s&15)+c>15? 0x40:0)|
		   ((0x80&(~r^s)&(s^(r+s+c)))? 4:0));
	  break;
	case 0xa:
	  /* FIXME: OV */
	  r = readmem(0x100);
	  s = rom[pc++];
	  pc &= 0xffff;
	  writemem(0x100, r-s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s<0? 0x80:0)|
		   ((r&15)-(s&15)<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xb:
	  /* FIXME: OV */
	  r = readmem(0x100);
	  s = rom[pc++];
	  pc &= 0xffff;
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r-s-c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s-c<0? 0x80:0)|
		   ((r&15)-(s&15)-c<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xc:
	  cy = 2;
	  writemem(0x100, rom[0xffff&(readmem(0x100)+readmem(0x104)+
				      (readmem(0x105)<<8))]);
	  break;
	case 0xd:
	  writemem(0x100, readmem(0x100)|rom[pc++]);
	  pc &= 0xffff;
	  break;
	case 0xe:
	  writemem(0x100, readmem(0x100)&rom[pc++]);
	  pc &= 0xffff;
	  break;
	case 0xf:
	  writemem(0x100, readmem(0x100)^rom[pc++]);
	  pc &= 0xffff;
	  break;
       }
       break;
     case 2:
     case 3:
       r = ((i&1)<<8)|rom[pc++];
       pc &= 0xffff;
       switch(i>>4) {
	case 0:
	  writemem(0x100, readmem(r));
	  break;
	case 1:
	  writemem(r, readmem(0x100));
	  break;
	case 2:
	  cy = 2;
	  writemem(r, rom[pc++]);
	  pc &= 0xffff;
	  break;
	case 3:
	  cy = 2;
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x101, (readmem(0x101)&0x7f)|(r<s? 0x80:0));
	  if(r == s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 4:
	  cy = 2;
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x101, (readmem(0x101)&0x7f)|(r<s? 0x80:0));
	  if(r != s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 5:
	  cy = 2;
	  s = (readlatch(r)-1)&0xff;
	  writemem(r, s);
	  if(s != 0)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 6:
	  writemem(r, readlatch(r)+1);
	  break;
	case 7:
	  writemem(r, readlatch(r)-1);
	  break;
	case 8:
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x100, r+s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s>255? 0x80:0)|
		   ((r&15)+(s&15)>15? 0x40:0)|((0x80&(~r^s)&(s^(r+s)))? 4:0));
	  break;
	case 9:
	  s = readmem(r);
	  r = readmem(0x100);
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r+s+c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s+c>255? 0x80:0)|
		   ((r&15)+(s&15)+c>15? 0x40:0)|
		   ((0x80&(~r^s)&(s^(r+s+c)))? 4:0));
	  break;
	case 0xa:
	  /* FIXME: OV */
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x100, r-s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s<0? 0x80:0)|
		   ((r&15)-(s&15)<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xb:
	  /* FIXME: OV */
	  s = readmem(r);
	  r = readmem(0x100);
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r-s-c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s-c<0? 0x80:0)|
		   ((r&15)-(s&15)-c<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xc:
	  s = readmem(r);
	  writemem(r, readmem(0x100));
	  writemem(0x100, s);
	  break;
	case 0xd:
	  writemem(0x100, readmem(0x100)|readmem(r));
	  break;
	case 0xe:
	  writemem(0x100, readmem(0x100)&readmem(r));
	  break;
	case 0xf:
	  writemem(0x100, readmem(0x100)^readmem(r));
	  break;
       }
       break;
     case 4:
     case 5:
     case 6:
     case 7:
       r = readmem((i&3)|((readmem(0x101)>>1)&0xc))|((i&2)? 0x100 : 0);
       switch(i>>4) {
	case 0:
	  writemem(0x100, readmem(r));
	  break;
	case 1:
	  writemem(r, readmem(0x100));
	  break;
	case 2:
	  writemem(r, rom[pc++]);
	  pc &= 0xffff;
	  break;
	case 3:
	  cy = 2;
	  s = readmem(r);
	  r = rom[pc++];
	  pc &= 0xffff;
	  writemem(0x101, (readmem(0x101)&0x7f)|(s<r? 0x80:0));
	  if(r == s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 4:
	  cy = 2;
	  s = readmem(r);
	  r = rom[pc++];
	  pc &= 0xffff;
	  writemem(0x101, (readmem(0x101)&0x7f)|(s<r? 0x80:0));
	  if(r != s)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 5:
	  cy = 2;
	  s = (readlatch(r)-1)&0xff;
	  writemem(r, s);
	  if(s != 0)
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 6:
	  writemem(r, readlatch(r)+1);
	  break;
	case 7:
	  writemem(r, readlatch(r)-1);
	  break;
	case 8:
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x100, r+s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s>255? 0x80:0)|
		   ((r&15)+(s&15)>15? 0x40:0)|((0x80&(~r^s)&(s^(r+s)))? 4:0));
	  break;
	case 9:
	  s = readmem(r);
	  r = readmem(0x100);
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r+s+c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r+s+c>255? 0x80:0)|
		   ((r&15)+(s&15)+c>15? 0x40:0)|
		   ((0x80&(~r^s)&(s^(r+s+c)))? 4:0));
	  break;
	case 0xa:
	  /* FIXME: OV */
	  s = readmem(r);
	  r = readmem(0x100);
	  writemem(0x100, r-s);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s<0? 0x80:0)|
		   ((r&15)-(s&15)<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xb:
	  /* FIXME: OV */
	  s = readmem(r);
	  r = readmem(0x100);
	  c = (readmem(0x101)&0x80)>>7;
	  writemem(0x100, r-s-c);
	  writemem(0x101, (readmem(0x101)&0x3b)|(r-s-c<0? 0x80:0)|
		   ((r&15)-(s&15)-c<0? 0x40:0)|(0? 4:0));
	  break;
	case 0xc:
	  s = readmem(r);
	  writemem(r, readmem(0x100));
	  writemem(0x100, s);
	  break;
	case 0xd:
	  writemem(0x100, readmem(0x100)|readmem(r));
	  break;
	case 0xe:
	  writemem(0x100, readmem(0x100)&readmem(r));
	  break;
	case 0xf:
	  writemem(0x100, readmem(0x100)^readmem(r));
	  break;
       }
       break;
     default:
       switch(i&0xe0) {
	case 0x00:
	  cy = 2;
	  r = ((i&7)<<8)|((i&0x10)<<7)|rom[pc++];
	  push(pc&0xff);
	  push((pc&0xff00)>>8);
	  pc = (pc&0xf000)|r;
	  break;
	case 0x20:
	  cy = 2;
	  r = ((i&7)<<8)|((i&0x10)<<7)|rom[pc++];
	  pc = (pc&0xf000)|r;
	  break;
	case 0x40:
	  cy = 2;
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  if((s=readmem(r))&(1<<(i&7))) {
	    writemem(r, s & ~(1<<(i&7)));
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  } else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 0x60:
	  cy = 2;
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  if(readmem(r)&(1<<(i&7)))
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 0x80:
	  cy = 2;
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  if(!(readmem(r)&(1<<(i&7))))
	    pc = 0xffff&(pc+1+SGNEXT(rom[pc]));
	  else {
	    pc++;
	    pc &= 0xffff;
	  }
	  break;
	case 0xa0:
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  writemem(r, readlatch(r) ^ (1<<(i&7)));
	  break;
	case 0xc0:
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  writemem(r, readlatch(r) & ~(1<<(i&7)));
	  break;
	case 0xe0:
	  r = ((i&0x10)<<4)|rom[pc++];
	  pc &= 0xffff;
	  writemem(r, readlatch(r) | (1<<(i&7)));
	  break;
       }
       break;
    }
    mcy += cy;
    if(mcy>=spd) {
      struct timeval now, t;
      if(lcd_updated)
	lcdrefresh();
      GETTIMEOFDAY(&now);
      if((epoch.tv_usec += 10000)>=1000000) {
	epoch.tv_usec -= 1000000;
	epoch.tv_sec++;
      }
      if(now.tv_sec>epoch.tv_sec ||
	 (now.tv_sec == epoch.tv_sec && now.tv_usec >= epoch.tv_usec)) {
	t.tv_usec = 0;
	t.tv_sec = 0;
      } else if(epoch.tv_usec<now.tv_usec) {
	t.tv_usec = 1000000 + epoch.tv_usec - now.tv_usec;
	t.tv_sec = epoch.tv_sec - now.tv_sec - 1;
      } else {
	t.tv_usec = epoch.tv_usec - now.tv_usec;
	t.tv_sec = epoch.tv_sec - now.tv_sec;
      }
      waitforevents(&t);
      checkevents();
      mcy -= spd;
      ++tick;
      if(tick>=50) {
	intreq |= 1<<3;
	tick -= 50;
      }
    }
    /* Timer 0 */
    if(sfr[0x10] & 0xc0) {
      int c0=0;
      if((t0base+=cy) >= t0scale)
	do
	  c0++;
	while((t0base-=t0scale) >= t0scale);
      if(c0)
	if((sfr[0x10] & 0xe0) == 0xe0) {
	  t0l += c0;
	  if(t0l>=256) {
	    t0l -= 256;
	    if(++t0h >= 256) {
	      t0h -= 256;
	      if((t0l += sfr[0x13])>=256) {
		t0l -= 256;
		if((t0h += sfr[0x15])>=256) {
		  t0l = sfr[0x13];
		  t0h = sfr[0x15];
		}
	      }
	      sfr[0x10] |= 10;
	      if(sfr[0x10]&4)
		intreq |= 1<<4;
	    }
	  }
	} else {
	  if(sfr[0x10] & 0x40) {
	    t0l += c0;
	    if(t0l>=256) {
	      t0l -= 256;
	      if((t0l += sfr[0x13])>=256)
		t0l = sfr[0x13];
	      sfr[0x10] |= 2;
	      if(sfr[0x10]&1)
		intreq |= 1<<2;
	    }
	  }
	  if(sfr[0x10] & 0x80) {
	    t0h += c0;
	    if(t0h>=256) {
	      t0h -= 256;
	      if((t0h += sfr[0x15])>=256)
		t0h = sfr[0x15];
	      sfr[0x10] |= 8;
	      if(sfr[0x10]&4)
		intreq |= 1<<4;
	    }
	  }
	}
    }
    /* Timer 1 */
    if(sfr[0x18] & 0xc0) {
      if((sfr[0x18] & 0xe0) == 0xe0) {
	t1l += cy;
	if(t1l>=256) {
	  t1l -= 256;
	  if(++t1h >= 256) {
	    t1h -= 256;
	    if((t1l += sfr[0x1b])>=256) {
	      t1l -= 256;
	      if((t1h += sfr[0x1d])>=256) {
		t1l = sfr[0x1b];
		t1h = sfr[0x1d];
	      }
	    }
	    sfr[0x18] |= 10;
	    if(sfr[0x18]&4)
	      intreq |= 1<<5;
	  }
	}
      } else {
	if(sfr[0x18] & 0x40) {
	  t1l += cy;
	  if(t1l>=256) {
	    t1l -= 256;
	    if((t1l += sfr[0x1b])>=256)
	      t1l = sfr[0x1b];
	    sfr[0x18] |= 2;
	    if(sfr[0x18]&1)
	      intreq |= 1<<5;
	  }
	}
	if(sfr[0x18] & 0x80) {
	  t1h += cy;
	  if(t1h>=256) {
	    t1h -= 256;
	    if((t1h += sfr[0x1d])>=256)
	      t1h = sfr[0x1d];
	    sfr[0x18] |= 8;
	    if(sfr[0x18]&4)
	      intreq |= 1<<5;
	  }
	}
      }
    }
    if(!(sfr[0x0d]&1) && !hasbios)
      if(!(pc=handle_fwcall(pc)))
	break;
      else
	sfr[0x0d]|=1;

    if((sfr[0x4e]&3)==3 && !imask)
      intreq |= 1<<9;

    if(!intreq || imask || !(sfr[0x08]&0x80))
      continue;
    for(r=0; r<10; r++)
      if(intreq & (1<<r))
	break;
    intreq &= ~(1<<r);
    push(pc&0xff);
    push((pc&0xff00)>>8);
    imask++;
    pc = ((r&~1)<<3)+((r&1)?0xb:0x3);
  }
}


int do_vmsgame(char *filename, char *biosname)
{
  if(filename == NULL && biosname == NULL)
    return 0;
  if(biosname != NULL)
    if(!loadbios(biosname))
      return 0;
  if(filename != NULL)
    if(!loadflash(filename))
      return 0;
  resetcpu();
  run_cpu();
  return 1;
}
