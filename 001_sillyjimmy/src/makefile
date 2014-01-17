CC=gcc
CFLAGS=-s -static -m32 -fno-stack-protector -static -O3 -std=c99 

SOURCES1=ctf_sillyjimmy_producer.c
EXECUTABLE1=sillyjimmy

SOURCES2=ctf_sillyjimmy_consumer.c
EXECUTABLE2=sillyjimmy_consumer

all: $(SOURCES1) $(EXECUTABLE1) $(SOURCES2) $(EXECUTABLE2)
	
$(EXECUTABLE1):
	$(CC) $(CFLAGS) -pthread $(SOURCES1) -o $@
    
$(EXECUTABLE2):
	$(CC) $(CFLAGS) $(SOURCES2) -o $@
    
clean:
	rm -f $(EXECUTABLE1) $(EXECUTABLE2)
