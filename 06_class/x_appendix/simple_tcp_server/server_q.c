/*******************************
 * Filename: server.c
 * Title: Simple TCP Server
 * Desc: Simple TCP Server
 *******************************/
 #include "inet.h"
 
 int main ( int argc, char* argv[] ) {
    int sockfd, newsockfd, clilen;
    struct sockaddr_in  cli_addr;
	struct sockaddr_in  serv_addr;
    char buff[30];
    pname = argv[0];
    
    /* TODO: sockfd에 AF_INET, SOCK_STREAM,0으로 소켓 지정
    
    /* 서버의 주소를 등록하여 클라이언트가 접속 가능하게 한다. */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl( INADDR_ANY );
    serv_addr.sin_port = htons( SERV_TCP_PORT );
    
    /* TODO: 주소를 바인딩 한다. (struct sockaddr *) &serv_addr */
    
    listen( sockfd, 5 );
    clilen = sizeof( cli_addr );
    
    /* TODO: accept()함수를 이용하여 주소를 연결한다. newsockfd */
    newsockfd = /* TODO */
    
    /* TODO: read()함수로 패킷을 읽어 온다. buff에 길이 20만큼 저장 한다. */

    printf("Server: Received String = %s \n", buff);
    close( sockfd );
    close( newsockfd );

    return 0;
}
/****************************************
 Run:
*****************************************
$ ./server.exe
Server: Received String = Hello world!~
****************************************/
