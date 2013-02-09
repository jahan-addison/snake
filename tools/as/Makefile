CC = gcc
LEX = flex
YACC = bison -y
RM = rm -f
CFLAGS = -g -Wall -pedantic -DDEBUG -DDEBUG_MALLOC # -O4711

TARGET = aslc86k
HASHSIZE = 12000

OBJS = main.o lexer.o gram.o file.o symbol.o storage.o macro.o smach.o \
		backend.o dmalloc.o getopt.o

all : $(TARGET)

clean :
	-$(RM) *.o *~ core
	-$(RM) $(TARGET) asmgen sieve
	-$(RM) lexer.c
	-$(RM) gram.c gram.h y.tab.c y.tab.h y.output
	-$(RM) aglexer.c aggram.c aggram.h

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@

sieve : sieve.o

hashsize.h : sieve
	echo "#define HASHSIZE `./sieve $(HASHSIZE)`" > hashsize.h

lexer.c : lexer.l
	$(LEX) -t $^ > $@

gram.c gram.h : gram.y
	$(YACC) -d -v $^ 
	mv y.tab.c gram.c
	mv y.tab.h gram.h

lexer.o : lexer.c gram.h taz.h smach.h

gram.o : gram.c gram.h taz.h smach.h backend.h

main.o : main.c taz.h smach.h backend.h

file.o : file.c taz.h smach.h

symbol.o : symbol.c taz.h hashsize.h

storage.o : storage.c taz.h

macro.o : macro.c taz.h

smach.o : smach.c taz.h smach.h backend.h

backend.o : backend.c taz.h smach.h backend.h

dmalloc.o : dmalloc.c

getopt.o : getopt.c

