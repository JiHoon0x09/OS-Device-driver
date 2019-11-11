/***************************************
 * Filename: echo_serv.c
 * Title: echo server
 * Desc: echo server Application
****************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXLINE 80
#define PORT    4000

void str_echo(int sockfd);
void sig_child(int signo);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    
    char cliaddrbuf[16] = {0};
    
    printf("This is echo server for snull!\n");
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
    listen(listenfd, 8);
    signal(SIGCHLD, sig_child);
    
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    printf("connfd: %d\n", connfd);
    close(connfd);
    close(listenfd);
}

void sig_child(int signo)
{
    pid_t pid;
    int stat;
    
    while((pid=waitpid(-1, &stat, WNOHANG))>0)
        printf("child %d terminated\n", pid);
    return;
}
