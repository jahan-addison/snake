#include <stdio.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE (0==0)
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef long numtype;


#ifdef DEBUG_MALLOC
extern int verbose_debug_malloc;
extern int verbose_debug_exit;
#define main dbm_main
extern void *debug_malloc(size_t, const char *, int);
extern void *debug_calloc(size_t, size_t, const char *, int);
extern void *debug_realloc(void *, size_t, const char *, int);
extern void debug_free(void *, const char *, int);
extern char *debug_strdup(const char *, const char *, int);
#define malloc(x) debug_malloc((x), __FILE__, __LINE__)
#define calloc(x, y) debug_calloc((x), (y), __FILE__, __LINE__)
#define realloc(x, y) debug_realloc((x), (y), __FILE__, __LINE__)
#define free(x) debug_free((x), __FILE__, __LINE__)
#define strdup(x) debug_strdup((x), __FILE__, __LINE__)
#endif

typedef int number;

struct mempool {
  void *block1;
  int allocsize, clustersize, headersize, totfree;
};

#define SYMB_UNDEF 0
#define SYMB_EQU   1
#define SYMB_SET   2
#define SYMB_MACRO 3
#define SYMB_RELOC 4
#define SYMB_UNDECIDED 5
#define SYMB_UNDSET 6

struct symbol {
  struct symbol *chain;
  char *name;
  int type;
  union {
    char *macro;
    number num;
  } value;
  const char *defined_file;
  int defined_line;
};

extern int current_lineno, numerrors, numlines;
extern const char *current_filename;
extern number *current_loc;
extern struct symbol *currloc_sym;

extern void errormsg(const char *, ...)
#ifdef __GNUC__
     __attribute__ ((format (printf, 1, 2)))
#endif
;

extern void initpool(struct mempool *, int, int);
extern void emptypool(struct mempool *);
extern void *poolallocn(struct mempool *, int);
extern void poolfreen(struct mempool *, void *, int);
extern char *poolstring(struct mempool *, const char *);

#define poolalloc(mp) poolallocn((mp),1)
#define poolfree(mp,a) poolfreen((mp),(a),1)
#define poolfreestr(mp, str) poolfreen((mp),(str),1+strlen((str)))

struct yy_buffer_state;
extern void file_init();
extern void file_end();
extern void process_buffer(const char *, int, struct yy_buffer_state *,
			   FILE *, void *);
extern int process_file(const char *);
extern int process_include(const char *);
extern void add_incdir(const char *);
extern int file_yywrap();
extern void filestack_push(int);
extern void filestack_pop();

extern void symbol_init();
extern void symbol_end();
extern struct symbol *lookup_symbol(const char *);
extern struct symbol *create_symbol(char *, int);
extern void init_symbols();
extern int symbol_check();
extern void flushlocalsyms();

extern void macro_init();
extern void macro_end();
extern struct symbol *findmacro(const char *);
extern void macro_storearg(int, const char *);
extern void invoke_macro(struct symbol *, int);
extern int abort_macro();
