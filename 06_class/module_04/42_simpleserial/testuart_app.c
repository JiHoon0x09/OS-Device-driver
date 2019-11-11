#include <stdio.h>
#include <fcntl.h>

int main(void)
{
	int fd, cnt;
	char buffer[1000];
	int i;
	
	fd = open("/dev/sser0", O_RDWR);

	if (fd<0) {
		printf("cannot open device sser0\n");
		exit(1);
	}
	printf("UART Test Program\n");

    strcpy( buffer, "UART0 Read Write test\n" );
    write( fd, buffer, strlen( buffer ) );
    
    for(i=0; i<5; i++) {
        strcpy( buffer, "\nInput string: " );
        write( fd, buffer, strlen( buffer ) );
        
        cnt = read( fd, buffer, 10 );
        
        buffer[cnt] = 0;
        printf( "Input string number: %d\n", cnt );
        printf( "Input string: %s\n", buffer );
        printf("\n");
        sleep(1);
    }
	
	close (fd);
}
