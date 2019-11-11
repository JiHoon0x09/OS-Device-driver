#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
	int fd;
	int retn;
	int flag=0;

	char wr_buf[100] = "write...\n";
	char rd_buf[100] = {0};

	fd = open("/dev/SK", O_RDWR);
	printf("fd = %d \n", fd);

	if(fd<0){
		perror("/dev/SK error");
		exit(-1);
	}else{
		printf("SK has been detected..\n");
	}

	//retn = write(fd, wr_buf, 10);
	printf("\nSize of written data : %d\n", retn);
	retn = read(fd, rd_buf, 20);
	printf("\nread data : %s \n", rd_buf);
	getchar();

	ioctl(fd,0,flag); getchar();
	ioctl(fd,1,flag); getchar();
	ioctl(fd,2,flag); getchar();
	ioctl(fd,3,flag); getchar();
	ioctl(fd,4,flag); getchar();

	close(fd);

	return 0;
}
