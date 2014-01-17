// compile with gcc -fno-stack-protector -static -O3 -s -std=c99 -o sillyjimmy ctf_sillyjimmy_producer.c

// perl -e 'print "\xF6\xFF\xFF\xFF\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x6a\x00\x79\x90\x6d\x90\x6d\x90\x69\x90\x00\x00\x00\x00\x01\x02\x03\x04\x01\x02\x03\x04\x3b\xc5\xc5\xc5" ' | nc 127.0.0.1 7331

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

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
    #define close _close
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

///////////////////////////////////////////////////////////////////////////////////

// global variables
char *consumer_cmd;
const char flag[] = "{dammit_I_thought_xor_encrpytion_was_unbreakable!}\n";
const char badsc[] = "Jimmy doesnt like your shellcode :/\n";
    
///////////////////////////////////////////////////////////////////////////////////

SOCKET init_producer()
{
    // startup a server socket
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = inet_addr("0.0.0.0");
    sain.sin_port = htons(PORT);
    bind(server_sock, (struct sockaddr*)&sain, sizeof(sain));
    
    return server_sock;
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

int findmagic(unsigned char* buf)
{
	if(buf[4] == 0xbc && buf[6] == 0xa8 && buf[8]== 0xa8
		&& buf[10] == 0xac && buf[12] == 0xaf)
		return 1;
	else
		return 0;
}

int open_pipe(int *readpipe, int *writepipe)
{
    int fdpipe[2]; // read, write
#if defined(WIN32) || defined(_WIN32)
    int r = pipe( fdpipe, BUFFER_SIZE, O_BINARY );
#else
    int r = pipe(fdpipe);
#endif
    *readpipe = fdpipe[0];
    *writepipe = fdpipe[1];
    return r;
}

void set_socket_timeout(SOCKET s, int sec)
{
    struct timeval tv;
    tv.tv_sec = sec; 
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
}

void *handle_producer_thread(void *context)
{
    char buf[BUFFER_SIZE];
    SOCKET clientsock = *(SOCKET*)context;
    free((SOCKET*)context);

    // set socket timeout to prevent resource hogging
    set_socket_timeout(clientsock, 5);
    
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
            send(clientsock, badsc, sizeof(badsc), 0);
        }
        else
        {
            // start consumer and open up a pipe
            printf("CMD: %s\n", consumer_cmd);
            int rdpipe, wrpipe;
            open_pipe(&rdpipe, &wrpipe);
            
            // on windows must open in binary mode
            FILE *consumerpipe = popen(consumer_cmd, OPEN_MODE_STR);
            
            // send the pipe handle
            fwrite(&wrpipe, sizeof(int), 1, consumerpipe);
            
            // send it to the consumer
            fwrite(buf, 1, recvlen, consumerpipe);
            fflush(consumerpipe);
            pclose(consumerpipe);
            
            recvlen = read(rdpipe, buf, sizeof(buf));
            if(recvlen > 0)
            {
                if(buf[0] == '\xFE')
                    send(clientsock, flag, sizeof(flag), 0); // send flag
                else
                    send(clientsock, buf, recvlen, 0);
            }
            
            // close pipes here
            close(rdpipe);
            close(wrpipe);
        }
    }
    closesocket(clientsock);
        
    return NULL;
}

void handle_producer(SOCKET server_sock)
{
    listen(server_sock, 5);
    while (1)
    {
        struct sockaddr_in sain;
        int size = sizeof(sain);
        SOCKET clientsock = accept(server_sock, (struct sockaddr*)&sain, &size);
        if (clientsock == INVALID_SOCKET)
        {
            // failed to connect to client, start over again
            printf("client failed to connect\n");
        }
        else
        {
            printf("client connected\n");
            // start new client thread
            pthread_t tid;
            SOCKET *psock = malloc(sizeof(SOCKET));
            *psock = clientsock;
            pthread_create(&tid, NULL, &handle_producer_thread, psock);
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

    // create the consumer command line
    consumer_cmd = malloc(strlen(argv[0]) + 10);
    strcpy(consumer_cmd, argv[0]);
    strcat(consumer_cmd, "_consumer");
        
    // run producer
    printf("PRODUCER started\n");
    SOCKET server_sock = init_producer();
    handle_producer(server_sock);
    
    // dont forget to free it
    free(consumer_cmd);
    
#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif
    return 0;
}

