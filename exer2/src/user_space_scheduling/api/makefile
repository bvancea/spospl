CC = gcc
CFLAGS = -g -O2 -Wall -Werror -DNDEBUG
SOURCES = fibo.c lists.c task.c
OBJECTS = fibo.o lists.o
BINARY = fibo
LIBRARIES = -lm

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(BINARY) $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is '$(BINARY)'.";
%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -f $(BINARY) $(OBJECTS) %.o
	