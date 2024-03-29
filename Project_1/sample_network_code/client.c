/* 
** CSSE2310/7231 - sample client - code to be commented in class
** Send a request for the top level web page (/) on some webserver and
** print out the response - including HTTP headers.
*   by Peter Sutton
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <netdb.h>
#include <string.h>

struct in_addr* name_to_IP_addr(char* hostname)
{
    int error;
    struct addrinfo* addressInfo;

    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if(error) {
    return NULL;
    }
    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

int connect_to(struct in_addr* ipAddress, int port)
{
    struct sockaddr_in socketAddr;
    int fd;
    
    // Create socket - IPv4 - TCP
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
    perror("Error creating socket");
    exit(1);
    }

    //Populate the address structure
    socketAddr.sin_family = AF_INET;    // IP v4 type address
    socketAddr.sin_port = htons(port);  // port num - network byte order
    socketAddr.sin_addr.s_addr = ipAddress->s_addr; // IP address
        // IP address is already in network byte order

    // Attempt to connect to server at that address
    if(connect(fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
    perror("Error connecting");
    exit(1);
    }

    return fd;
}

void send_HTTP_request(int fd, char* file, char* host)
{
    char* requestString;

    // Allocate enough space for our HTTP request
    requestString = (char*)malloc(strlen(file) + strlen(host) + 26);

    // Construct HTTP request
    // GET (file) HTTP/1.0
    // Host: hostname
    // (blank line)
    sprintf(requestString, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", file, host);

    // Send our request to server - write to fd
    if(write(fd, requestString, strlen(requestString)) < 1) {
    perror("Write error");
    exit(1);
    }
}

void get_and_output_HTTP_response(int fd)
{
    char buffer[1024];
    int numBytesRead;
    int eof = 0;

    // Repeatedly read from network fd until nothing left (EOF)
    // Write everything out to stdout
    while(!eof) {
    numBytesRead = read(fd, buffer, 1024);
    if(numBytesRead < 0) {
        perror("Read error\n");
        exit(1);
    } else if(numBytesRead == 0) {
        eof = 1;
    } else {
        fwrite(buffer, sizeof(char), numBytesRead, stdout);
    }
    }
}

int main(int argc, char* argv[]) {
    int fd;
    struct in_addr* ipAddress;
    char* hostname;

    if(argc != 2) {
    fprintf(stderr, "Usage: %s hostname\n", argv[0]);
    exit(1);
    }
    hostname = argv[1];

    // Convert host name to IP address
    ipAddress = name_to_IP_addr(hostname);
    if(!ipAddress) {
    fprintf(stderr, "%s is not a valid hostname\n", hostname);
    exit(1);
    }

    // Connect to this host on port 80
    fd = connect_to(ipAddress, 80);
    // Send our HTTP request for the top level page, print the response
    send_HTTP_request(fd, "/", hostname);
    get_and_output_HTTP_response(fd);
    close(fd);
    return 0;
}