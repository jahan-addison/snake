extern void waitforevents(struct timeval *);
extern void checkevents();
extern void redrawlcd();
extern void vmputpixel(int, int, int);
extern void keypress(int i);
extern void keyrelease(int i);
extern int do_vmsgame(char *filename, char *biosname);
extern int do_lcdimg(char *filename);
extern void sound(int freq);
#ifdef BSD_STYLE_GETTIMEOFDAY
#ifdef TIMEZONE_IS_VOID
extern int gettimeofday(struct timeval *, void *);
#else
extern int gettimeofday(struct timeval *, struct timezone *);
#endif
#define GETTIMEOFDAY(tp) gettimeofday(tp, NULL)
#else
extern int gettimeofday(struct timeval *);
#define GETTIMEOFDAY(tp) gettimeofday(tp)
#endif
extern void error_msg(char *fmt, ...);
