/***************************************
 * Filename: echo_client.c
 * Title: echo client
 * Desc: echo client Application
****************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MAXLINE 80
#define PORT    4000
#define ADDR    "192.168.1.57"

void str_cli(FILE *, int );

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    char servaddrbuf[16] = {0};
    
    printf("This is echo client for snull!\n");
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if(argc == 2)
        inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    else
        inet_pton(AF_INET, ADDR, &servaddr.sin_addr);
    
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
    exit(0);
    
}
