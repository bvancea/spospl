CC = gcc
CFLAGS = -g -O2 -Wall
BINARY = random_generator
OBJECTS = random_generator.o

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARY) -ldl -pthread

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(BINARY) $(OBJECTS)
