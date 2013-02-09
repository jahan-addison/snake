#include <stdio.h>
#include <ronin/vmsfs.h>

extern int openmainwin();

extern unsigned char flash[0x20000];
extern void run_cpu();
extern void resetcpu();
extern void check_gamesize();
extern int gamesize;
extern int selectvm();
extern void unselectvm();

extern void display_progress_bar(float p);

static struct vmsinfo info;

int flash_written(int a)
{
  int block = a>>9;
  return vmsfs_write_block(&info, block, flash+(block<<9));
}

int vmloadflash(struct vmsinfo *info)
{
  int i;
  memset(flash, 0, sizeof(flash));
  for(i=0; i<256; i++) {
    if(info == NULL || !vmsfs_read_block(info, i, flash+(i<<9))) {
      while(i<64)
	display_progress_bar((1.0/256)*(i++));
      return 0;
    }
    display_progress_bar((1.0/256)*i);
  }
  gamesize = 0;
  check_gamesize();
  printf("Game size = %d\n", gamesize);
  return gamesize>0;
}

void bootmenu()
{
  (*(void(**)())0x8c0000e0)(0);
  for(;;);
}

int main()
{
  struct vmsinfo *ii;

#ifndef NOSERIAL
  serial_init(57600);
  usleep(2000000);
  printf("Serial OK\r\n");
#endif
  maple_init();
  init_arm();

  if(!openmainwin())
    bootmenu();

  for(;;) {
    int vmn;

    if((vmn = selectvm()) < 0)
      break;

    vmn = (vmn&3)*6+(vmn>>2)+1;

    if(vmsfs_check_unit(vmn, 0, &info))
      ii = &info;
    else
      ii = NULL;

    if(vmloadflash(ii)) {
      redrawlcd();
      resetcpu();
      run_cpu();
      stop_sound();
    }

    unselectvm();
  }
  bootmenu();
}
