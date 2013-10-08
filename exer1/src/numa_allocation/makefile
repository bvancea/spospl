CC = gcc
CFLAGS = -g -O2 -Wall -Werror
BINARY = numa_allocation
OBJECTS = numa_allocation.o
LIBRARIES = -pthread -lnuma

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARY) -ldl $(LIBRARIES)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(BINARY) $(OBJECTS)