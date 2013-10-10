CC = @gcc
CFLAGS = -g -O2 -Wall -Werror
BINARY = false_sharing
OBJECTS = false_sharing.o

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BINARY) -ldl -pthread
	@echo "False Sharing done."
%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -f $(BINARY) $(OBJECTS)
