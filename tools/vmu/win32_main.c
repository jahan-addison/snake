#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include "version.h"
#include "prototypes.h"

extern HWND hWnd;

extern int openmainwin(HINSTANCE inst);
extern void deinitmainwin(HINSTANCE inst);

char fileName[1024];

extern int pixdbl;

extern int optind;
extern char *optarg;
extern int getopt(int argc, char * const * argv, const char *optstring);

void error_msg(char *fmt, ...)
{
  char buf[1024];
  va_list va;
  va_start(va, fmt);
  _vsnprintf(buf, sizeof(buf)-1, fmt, va);
  buf[sizeof(buf)-1]='\0';
  MessageBox(NULL, buf, "SoftVMS " VERSION, 0);
  va_end(va);
}

char *ShowOpenDialog(HINSTANCE hInstance)
{
  OPENFILENAME ofn = {
    sizeof(OPENFILENAME),
    NULL,
    hInstance,
    "VMS Game files (*.vms)\0*.vms\0LCD Image files (*.lcd)\0*.lcd\0All Files (*.*)\0*.*\0",
    NULL, 0, 0,
    fileName, 1024,
    NULL, 0,
    NULL,
    "Open VMS Game",
    OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
    0, 0, NULL,
    0, NULL, NULL
  };
  fileName[0] = 0;
  return GetOpenFileName(&ofn)?fileName:NULL;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  int c, argc, r, fd;
  char *bios = NULL;
  char hdr[4];
  LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
  char **argv = calloc(argc+1, sizeof(char *));
  for(c=0; c<argc; c++) {
    int l = wcslen(argvw[c]);
    wcstombs(argv[c] = malloc(2*(l+1)), argvw[c], 2*(l+1));
  } 
  
  while((c=getopt(argc, argv, "hV2b:"))!=EOF)
    switch(c) {
     case 'h':
       error_msg("Usage: %s [-h] [-V] [-2] [-b bios] game.vms ...", argv[0]);
       break;
     case 'V':
       MessageBox(NULL, "softvms " VERSION " by Marcus Comstedt <marcus@idonex.se>\nand David Steinberg <d.steinberg@ardeo.com>", "SoftVMS " VERSION, 0);
       break;
     case '2':
       pixdbl = 1;
       break;
     case 'b':
       bios = optarg;
       break;
    }

  if(argc<=optind)
    if((argv[argc] = ShowOpenDialog(hInstance)) == NULL)
      return 0;
    else
      optind = argc++;
  
  if(!openmainwin(hInstance))
    return -1;
  
  fd = open(argv[optind], O_RDONLY);
  if(fd<0 || (r=read(fd, hdr, 4)<4)) {
    perror(argv[optind]);
    if(fd>=0)
      close(fd);
    deinitmainwin(hInstance);
    return -1;
  }
  if(hdr[0]=='L' && hdr[1]=='C' && hdr[2]=='D' && hdr[3]=='i')
    do_lcdimg(argv[optind]);
  else
    do_vmsgame(argv[optind], bios);
  deinitmainwin(hInstance);
  return 0;
}
