CC = gcc
CFLAGS = -g -O2 -Wall -Werror -D_GNU_SOURCE -DNDEBUG
FIB_SOURCES = fibo.c task.c list.c talloc.c
FIB_OBJECTS = fibo.o 
FIB_BINARY = fibo

LIBRARIES = -lm -pthread 

all:
	make fibo;
	make fibo_tcmalloc;
	make fibo_malloc;

fibo:
	$(CC) $(CFLAGS) $(FIB_SOURCES) -o $(FIB_BINARY) $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is '$(FIB_BINARY)'.";

fibo_tcmalloc:
	$(CC) $(CFLAGS) $(FIB_SOURCES) -o fibo_tcmalloc $(LIBRARIES) -ltcmalloc 
	@echo "Compilation succesfull. The binary generated is fibo_tcmalloc.";

fibo_malloc:
	$(CC) $(CFLAGS) -DUSE_MALLOC $(FIB_SOURCES) -o fibo_malloc $(LIBRARIES)
	@echo "Compilation succesfull. The binary generated is fibo_malloc.";

%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -f $(GRT_BINARY) $(GRT_OBJECTS) $(FIB_BINARY) $(FIB_OBJECTS) %.o
	