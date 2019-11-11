/*******************************
 * Filename: client.c
 * Title: Simple TCP Server
 * Desc: Simple TCP Server
 *******************************/
 #include "inet.h"
 
 int main ( int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    pname = argv[0];
    
    /* 연결하고자 하는 서버의 주소 및 포트번호를 구조체 serv_addr에 채운다 */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr( SERV_HOST_ADDR );
    serv_addr.sin_port = htons( SERV_TCP_PORT );
    
    /* TCP Socket Open */
    if ((sockfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ) {
        puts( "Client: 소켓을 열수 없습니다.");
        exit(1);
    }
    
    /* 서버에 접속 */
    if ( connect( sockfd, (struct sockaddr *) &serv_addr, sizeof( serv_addr )) < 0 ) {
        puts( "Client: 서버에 연결할 수 없습니다.");
        exit(1);
    }
    
    if ( write( sockfd, "Hello world!~", 20) < 20 ) {
        puts( "Client: writen error" );
        exit(1);
    }
    
    close( sockfd );
}
