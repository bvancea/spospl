CC = gcc
CFLAGS = -g -O2 -Wall -Werror -DNDEBUG
FIB_SOURCES = fibo.c lists.c task.c
FIB_OBJECTS = fibo.o lists.o
FIB_BINARY = fibo

GRT_SOURCES = printer.c task.c lists.c
GRT_OBJECTS = printer.o lists.o
GRT_BINARY = printer

LIBRARIES = -lm

all:
	make fibo;
	make printer;

fibo:
	$(CC) $(CFLAGS) $(FIB_SOURCES) -o $(FIB_BINARY) $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is '$(FIB_BINARY)'.";
	
printer:
	$(CC) $(CFLAGS) $(GRT_SOURCES) -o $(GRT_BINARY) $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is '$(GRT_BINARY)'.";
	
%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -f $(GRT_BINARY) $(GRT_OBJECTS) $(FIB_BINARY) $(FIB_OBJECTS) %.o
	