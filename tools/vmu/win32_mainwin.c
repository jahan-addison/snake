#include <windows.h>
#include <sys/timeb.h>
#include <stdio.h>
#include "version.h"
#include "prototypes.h"

#include "vmu.xpm"
#include "vmu.5.xpm"

#define FGCOLOR RGB(0x08,0x10,0x52)
#define BGCOLOR RGB(0xaa,0xd5,0xc3)

#define STYLES WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE|WS_POPUP

ATOM clsAtom = 0;
HWND hWnd = NULL;
HDC hDC;
MSG msg;
HBITMAP hBitmap;
HBITMAP bgimg;
HDC hDCbuff;
int img_w, img_h;

int pixdbl;

void low_redrawlcd(HDC hDC)
{
  if(pixdbl)
    BitBlt(hDC, 33, 86, 96, 80, hDCbuff, 0, 0, SRCCOPY);
  else
    BitBlt(hDC, 17, 43, 48, 40, hDCbuff, 0, 0, SRCCOPY);
}

void redrawlcd()
{
  low_redrawlcd(hDC);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
  
  switch (uMsg)
    {
     case WM_KEYDOWN:
       switch(wParam & 0xff) {
	case VK_UP:
	  keypress(0);
	  break;
	case VK_DOWN:
	  keypress(1);
	  break;
	case VK_LEFT:
	  keypress(2);
	  break;
	case VK_RIGHT:
	  keypress(3);
	  break;
	case VK_CONTROL:
	case 'A':
	  keypress(4);
	  break;
	case VK_MENU:
	case 'B':
	  keypress(5);
	  break;
	case VK_RETURN:
	case 'M':
	  keypress(6);
	  break;
	case VK_SPACE:
	case 'S':
	  keypress(7);
	  break;
	default:
	  return DefWindowProc(hwnd, uMsg, wParam, lParam);
       }
       return 0;
       
     case WM_KEYUP:
       switch(wParam & 0xff) {
	case VK_UP:
	  keyrelease(0);
	  break;
	case VK_DOWN:
	  keyrelease(1);
	  break;
	case VK_LEFT:
	  keyrelease(2);
	  break;
	case VK_RIGHT:
	  keyrelease(3);
	  break;
	case VK_CONTROL:
	case 'A':
	  keyrelease(4);
	  break;
	case VK_MENU:
	case 'B':
	  keyrelease(5);
	  break;
	case VK_RETURN:
	case 'M':
	  keyrelease(6);
	  break;
	case VK_SPACE:
	case 'S':
	  keyrelease(7);
	  break;
	default:
	  return DefWindowProc(hwnd, uMsg, wParam, lParam);
       }
       return 0;
       
     case WM_CLOSE:
       DestroyWindow(hWnd);
       return 0;
       
     case WM_DESTROY:
       PostQuitMessage(0);
       return 0;
       
     case WM_PAINT:
       hdc = BeginPaint(hwnd, &ps); 
       SelectObject(hDCbuff, bgimg);	
       BitBlt(hdc, 0, 0, img_w, img_h, hDCbuff, 0, 0, SRCCOPY);
       SelectObject(hDCbuff, hBitmap);
       low_redrawlcd(hdc);
       EndPaint(hwnd, &ps); 
       return 0L; 
       
     default:
       return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
  return 0;
}

WNDCLASS wnd_class = {
  0,
  MainWndProc,
  0, 0,
  NULL,
  NULL,
  NULL,
  (HBRUSH)COLOR_BTNFACE,
  NULL,
  "softvms_wnd_class"
};

static HBITMAP mkbgpixmap(char **vmu1_xpm, int *wret, int *hret)
{
  int i, w, h, nc, x, y;
  COLORREF pixel[256];
  HBITMAP img;
  HDC dc;
  sscanf(vmu1_xpm[0], "%d %d %d", &w, &h, &nc);
  for(i=1; i<=nc; i++) {
    int r, g, b;
    sscanf(vmu1_xpm[i]+4, "#%2x%2x%2x", &r, &g, &b);
    pixel[*(unsigned char *)vmu1_xpm[i]] = RGB(r, g, b);
  }
  dc = GetDC(NULL);
  if((img = CreateCompatibleBitmap(dc, w, h))==NULL) {
    error_msg("Failed to allocate BitMap");
    return NULL;
  }
  dc = CreateCompatibleDC(dc);
  SelectObject(dc, img);
  for(y=0; y<h; y++) {
    unsigned char *p = (unsigned char *)vmu1_xpm[i++];
    for(x=0; x<w; x++)
      SetPixel(dc, x, y, pixel[*p++]);
  }
  DeleteDC(dc);
  *wret = w;
  *hret = h;
  return img;
}

int openmainwin(HINSTANCE inst)
{
  int w, h;
  RECT r;
  
  wnd_class.hInstance = inst;
  clsAtom = RegisterClass(&wnd_class);
  if(!clsAtom)
    return 0;
  
  if((bgimg = mkbgpixmap((pixdbl? vmu_large:vmu_small), &w, &h))==NULL)
    return 0;
  
  r.left = r.top = 20;
  r.right = r.left + w;
  r.bottom = r.top + h;
  AdjustWindowRect(&r, STYLES, FALSE);
  
  hWnd = CreateWindow("softvms_wnd_class", "SoftVMS " VERSION, STYLES,
		      CW_USEDEFAULT, CW_USEDEFAULT, r.right-r.left, r.bottom-r.top, NULL, NULL, inst, NULL);
  
  if(hWnd == NULL)
    return 0;
  
  hDC = GetDC(hWnd);
  
  hDCbuff = CreateCompatibleDC(hDC);
  SelectObject(hDCbuff, bgimg);	
  BitBlt(hDC, 0, 0, img_w = w, img_h = h, hDCbuff, 0, 0, SRCCOPY);
  
  hBitmap = CreateCompatibleBitmap(hDC, (pixdbl? 96:48), (pixdbl? 80:40));
  SelectObject(hDCbuff, hBitmap);	
  SelectObject(hDCbuff, CreateSolidBrush(BGCOLOR));
  PatBlt(hDCbuff, 0, 0, (pixdbl? 96:48), (pixdbl? 80:40), PATCOPY);
  
  return 1;
}

void deinitmainwin(HINSTANCE inst)
{
  UnregisterClass("softvms_wnd_class", inst);
}

void vmputpixel(int x, int y, int p)
{
  if(pixdbl) {
    x<<=1;
    y<<=1;
    SetPixel(hDCbuff, x, y, p&1?FGCOLOR:BGCOLOR);
    SetPixel(hDCbuff, x+1, y, p&1?FGCOLOR:BGCOLOR);
    SetPixel(hDCbuff, x, y+1, p&1?FGCOLOR:BGCOLOR);
    SetPixel(hDCbuff, x+1, y+1, p&1?FGCOLOR:BGCOLOR);
  } else
    SetPixel(hDCbuff, x, y, p&1?FGCOLOR:BGCOLOR);
}

void checkevents() {
  while(PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE)) {
    if(msg.message == WM_QUIT) {
      deinitmainwin(wnd_class.hInstance);
      exit(0);
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void waitforevents(struct timeval *tv)
{
  DWORD millis = INFINITE;
  if(tv != NULL)
    millis = tv->tv_sec * 1000 + tv->tv_usec / 1000;
  MsgWaitForMultipleObjects(0, NULL, FALSE, millis, QS_ALLINPUT);
}

/* UNIX compat... */

int gettimeofday(struct timeval *tv)
{
  struct _timeb timebuffer;
  _ftime( &timebuffer );
  tv->tv_sec = timebuffer.time;
  tv->tv_usec = timebuffer.millitm*1000;
  return 0;
}

void sleep(int t)
{
  Sleep(t*1000);
}

void sound(int freq)
{
}
