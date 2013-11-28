CC = gcc
CFLAGS = -g -O2 -Wall -Werror -D_GNU_SOURCE 
FIB_SOURCES = fibo.c task.c
FIB_OBJECTS = fibo.o 
FIB_BINARY = fibo

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
	