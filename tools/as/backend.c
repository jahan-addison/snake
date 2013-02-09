#include <stdio.h>
#include <string.h>

#include "taz.h"
#include "smach.h"
#include "backend.h"

#define SECTIONSLICESZ (0x10000)

int *be_holdbits;
SMV *be_holdSMVs;
int be_freehold;
static int allochold;
static int *bitbase;
static SMV *SMVbase;

struct section builtin_section, *all_sections, *emit_section;
int num_sections;
int allocated_sections;


void begen(SMV v, int b)
{
  int used = allochold-be_freehold;
  for(;;) {
    smach_emit(v, b);
    if(used<0)
      break;
    b = *--be_holdbits;
    v = *--be_holdSMVs;
    --used;
  }
  be_freehold = allochold+1;
  smach_flush();
}

void behold(SMV v, int b)
{
  if(allochold != 0) {
    be_freehold = allochold;
    allochold <<= 1;
    be_holdbits = (bitbase = realloc(bitbase, sizeof(*bitbase)*allochold)) +
      be_freehold;
    be_holdSMVs = (SMVbase = realloc(SMVbase, sizeof(*SMVbase)*allochold)) +
      be_freehold;
  } else {
    be_freehold = allochold = 10;
    be_holdbits = bitbase = malloc(sizeof(*bitbase)*allochold);
    be_holdSMVs = SMVbase = malloc(sizeof(*SMVbase)*allochold);
  }
  if(bitbase == NULL || SMVbase == NULL) {
    fprintf(stderr, "Fatal: Out of memory!\n");
    exit(2);
  }
  *be_holdbits++=b;
  *be_holdSMVs++=v;
}



static void close_slice()
{
  int sliceno = emit_section->numslices;

  if(!sliceno)
    return;

  --sliceno;

  emit_section->slices[sliceno].used =
    emit_section->slices[sliceno].allocated - emit_section->left;

  emit_section->size += emit_section->slices[sliceno].used;
}

void be_extend(int b)
{
  int sliceno;
  close_slice();
  if((sliceno=emit_section->numslices) >= emit_section->allocslices) {
    int old_alloced;
    if((old_alloced=emit_section->allocslices)!=0)
      emit_section->slices = realloc(emit_section->slices,
			     (emit_section->allocslices<<=1)*
			     sizeof(struct section_slice));
    else
      emit_section->slices = malloc((emit_section->allocslices=10)*
			    sizeof(struct section_slice));
    if(emit_section->slices == NULL) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }

    memset(&emit_section->slices[old_alloced], 0,
	   (emit_section->allocslices-old_alloced)*sizeof(struct section_slice));
  }
  if(emit_section->slices[sliceno].allocated < b) {
    if(emit_section->slices[sliceno].allocated)
      free(emit_section->slices[sliceno].base);
    if(b<SECTIONSLICESZ)
      b=SECTIONSLICESZ;
    if((emit_section->slices[sliceno].base =
	malloc(emit_section->slices[sliceno].allocated = b)) == NULL) {
      fprintf(stderr, "Fatal: Out of memory!\n");
      exit(2);
    }
  }
  emit_section->ptr = emit_section->slices[sliceno].base;
  emit_section->left = emit_section->slices[sliceno].allocated;
  emit_section->numslices++;
}

#ifdef SUBBYTE_EMIT

#error not implemented

#else

void be_emitn(int bi, numtype n)
{
  int by=(bi+7)>>3;
  int all_by = by;
  if(emit_section->left<by)
    be_extend(by);
  emit_section->left -= by;
  *current_loc += by;
  while(by--) {
    emit_section->ptr[by]=n;
    n>>=8;
  }
  emit_section->ptr += all_by;
}

void be_emiti(int by, unsigned char *p)
{
  if(emit_section->left<by)
    be_extend(by);
  emit_section->left -= by;
  *current_loc += by;
  while(by--)
    *emit_section->ptr++ = *p++;
}

#endif



int write_section_data(struct section *s, FILE *f)
{
  int n;
  for(n=0; n<s->numslices; n++)
    if(fwrite(s->slices[n].base, 1, s->slices[n].used, f) !=
       s->slices[n].used)
      return 0;
  return 1;
}

int write_section_srec(struct section *s, FILE *f, char *tmpl)
{
  int cs, i, n, l, a=0, p;
  if(tmpl == NULL)
    tmpl = "S1%02X%04X";
  for(n=0; n<s->numslices; n++)
    for(p=0; p<s->slices[n].used; a+=l, p+=l) {
      if((l = s->slices[n].used-p)>16)
	l = 16;
      if(fprintf(f, tmpl, l, a)<0)
	return 0;
      cs = l + ((a&0xff000000)>>24) + ((a&0xff0000)>>16) + ((a&0xff00)>>8) + (a&0xff);
      for(i=0; i<l; i++) {
	int b = ((unsigned char *)s->slices[n].base)[p+i];
	cs += b;
	if(fprintf(f, "%02X", b)<0)
	  return 0;
      }
      if(fprintf(f, "%02X\n", (-cs)&0xff)<0)
	return 0;
    }
  return 1;
}


static void free_section(struct section *s)
{
  int n;

  for(n=0; n<s->allocslices; n++)
    if(s->slices[n].allocated)
      free(s->slices[n].base);

  if(s->allocslices)
    free(s->slices);
}

static void reset_section(struct section *s)
{
  s->size = 0;
  s->numslices = 0;
  s->left = 0;
}

static void close_emit_section()
{
  close_slice();
  *current_loc = 0;
}

void backend_finalize()
{
  close_emit_section();
}

void reset_sections()
{
  int n;
  reset_section(&builtin_section);
  for(n=0; n<num_sections; n++)
    reset_section(&all_sections[n]);
  if(current_loc != NULL)
    *current_loc = 0;
}

void backend_init()
{
  be_holdbits = bitbase = NULL;
  be_holdSMVs = SMVbase = NULL;
  allochold = 0;
  be_freehold = 1;
  allocated_sections = num_sections = 0;
  all_sections = NULL;
  builtin_section.name = "DEFAULT";
  builtin_section.allocslices = 0;
  reset_sections();
  emit_section = &builtin_section;
}

void backend_end()
{
  if(bitbase) free(bitbase);
  if(SMVbase) free(SMVbase);
  free_section(&builtin_section);
  if(allocated_sections) {
    int n;
    for(n=0; n<num_sections; n++) {
      free_section(&all_sections[n]);
      free(all_sections[n].name);
    } 
    free(all_sections);
  }
}
