CC = gcc
CFLAGS = -g -O2 -Wall -Werror
LIBRARIES = -lm

SETJMP_SOURCES = setjmp_test.c utils.c
SETJMP_OBJECTS = setjmp_test.o utils.o
SETJMP_BINARY = setjmp_test

UCONTEXT_SOURCES = ucontext_test.c utils.c
UCONTEXT_OBJECTS = ucontext_test.o utils.o
UCONTEXT_BINARY = ucontext_test

setjmp:
	$(CC) $(CFLAGS) $(SETJMP_SOURCES) -o $(SETJMP_BINARY) $(LIBRARIES)

ucontext:
	$(CC) $(CFLAGS) $(UCONTEXT_SOURCES) -o $(UCONTEXT_BINARY) $(LIBRARIES) -m32
		
all:
	make setjmp;
	make ucontext;

clean:
	@rm -f $(SETJMP_BINARY) $(UCONTEXT_BINARY)%.o