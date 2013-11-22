CC = gcc
CFLAGS = -g -O2 -Wall -Werror
FIB_SOURCES = fibo.c lists.c task.c
FIB_OBJECTS = fibo.o lists.o
FIB_BINARY = fibo

GRT_SOURCES = printer.c task.c lists.c 
GRT_OBJECTS = printer.o lists.o
GRT_BINARY = printer

LIBRARIES = -lm -pthread 

all:
	make fibo;

fibo:
	$(CC) $(CFLAGS) $(FIB_SOURCES) -o $(FIB_BINARY) $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is '$(FIB_BINARY)'.";

%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -f $(GRT_BINARY) $(GRT_OBJECTS) $(FIB_BINARY) $(FIB_OBJECTS) %.o
	