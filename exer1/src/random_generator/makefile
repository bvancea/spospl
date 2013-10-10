CC = @gcc
CFLAGS = -g -O2 -Wall -Werror
BINARY = random_generator
OBJECTS = random_generator.o

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARY) -ldl -pthread
	@echo "Random Generator done."
%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	@rm -f $(BINARY) $(OBJECTS)
