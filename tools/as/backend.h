struct section_slice {
  unsigned long allocated;
  unsigned long used;
  void *base;
};

extern struct section {
  char *name;
  unsigned long size;
  int numslices, allocslices;
  struct section_slice *slices;
  unsigned char *ptr;
  unsigned long left;
} *emit_section, builtin_section;

extern void backend_init();
extern void backend_end();
extern void backend_finalize();
extern void begen(SMV v, int b);
extern void behold(SMV v, int b);
extern void reset_sections();
extern int write_section_data(struct section *s, FILE *f);
extern int write_section_srec(struct section *s, FILE *f, char *tmpl);

extern int be_freehold;
extern int *be_holdbits;
extern SMV *be_holdSMVs;

#define GEN(x,b) begen(x,b)
#define HOLD(x,b) ((--be_freehold)?(((void)(*be_holdbits++=b)),((void)(*be_holdSMVs++=x))):behold(x,b))

extern void be_emitn(int, numtype);
extern void be_emiti(int, unsigned char *);
extern void be_extend(int);

#define EMITN(bi,n) be_emitn(bi, n)
#define EMITI(by,p) be_emiti(by, p)

#ifdef SUBBYTE_EMIT

#define EMIT1(n) EMITN(1,(n))
#define EMIT2(n) EMITN(2,(n))
#define EMIT3(n) EMITN(3,(n))
#define EMIT4(n) EMITN(4,(n))
#define EMIT5(n) EMITN(5,(n))
#define EMIT6(n) EMITN(6,(n))
#define EMIT7(n) EMITN(7,(n))

#define EMIT8(n) EMITN(8,(n))
#define EMIT16(n) EMITN(16,(n))
#define EMIT24(n) EMITN(24,(n))
#define EMIT32(n) EMITN(32,(n))
#define EMIT40(n) EMITN(40,(n))
#define EMIT48(n) EMITN(48,(n))
#define EMIT56(n) EMITN(56,(n))

#else

#define EMIT1(n) EMIT8(n)
#define EMIT2(n) EMIT8(n)
#define EMIT3(n) EMIT8(n)
#define EMIT4(n) EMIT8(n)
#define EMIT5(n) EMIT8(n)
#define EMIT6(n) EMIT8(n)
#define EMIT7(n) EMIT8(n)

#define EMIT8(n) {if(emit_section->left<1)be_extend(1);\
  *emit_section->ptr++=(n);emit_section->left--;(*current_loc)++;}
#define EMIT16(n) {numtype _=(n);if(emit_section->left<2)be_extend(2);\
  *emit_section->ptr++=_>>8;*emit_section->ptr++=_;emit_section->left-=2;\
  (*current_loc)+=2;}
#define EMIT24(n) {numtype _=(n);if(emit_section->left<3)be_extend(3);\
  *emit_section->ptr++=_>>16;*emit_section->ptr++=_>>8;\
  *emit_section->ptr++=_;emit_section->left-=3;(*current_loc)+=3;}
#define EMIT32(n) {numtype _=(n);if(emit_section->left<4)be_extend(4);\
  *emit_section->ptr++=_>>24;*emit_section->ptr++=_>>16;\
  *emit_section->ptr++=_>>8;*emit_section->ptr++=_;emit_section->left-=4;\
  (*current_loc)+=4;}
#define EMIT40(n) {numtype _=(n);if(emit_section->left<5)be_extend(5);\
  *emit_section->ptr++=_>>32;*emit_section->ptr++=_>>24;\
  *emit_section->ptr++=_>>16;*emit_section->ptr++=_>>8;\
  *emit_section->ptr++=_;emit_section->left-=5;(*current_loc)+=5;}
#define EMIT48(n) {numtype _=(n);if(emit_section->left<6)be_extend(6);\
  *emit_section->ptr++=_>>40;*emit_section->ptr++=_>>32;\
  *emit_section->ptr++=_>>24;*emit_section->ptr++=_>>16;\
  *emit_section->ptr++=_>>8;*emit_section->ptr++=_;emit_section->left-=6;\
  (*current_loc)+=6;}
#define EMIT56(n) {numtype _=(n);if(emit_section->left<7)be_extend(7);\
  *emit_section->ptr++=_>>48;*emit_section->ptr++=_>>40;\
  *emit_section->ptr++=_>>32;*emit_section->ptr++=_>>24;\
  *emit_section->ptr++=_>>16;*emit_section->ptr++=_>>8;\
  *emit_section->ptr++=_;emit_section->left-=7;(*current_loc)+=7;}

#endif

