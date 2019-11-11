#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<linux/ioctl.h>

int main(void){
	int retn;
	char buf[100] = {0};
	int fd;

	fd = open("/dev/sk",O_RDWR);
	printf("fd = %d \n\n",fd);

	if(fd < 0){
		perror("/dev/sk ERROR!!\n");
		exit(-1);
	}
	else
		printf("sk module has been detected...\n");

	printf("USER: read data\n");
	//fd 가르키는 파일의 buf에서 20바이트 읽음
	retn = read(fd, buf, 20);
	
	//retn = write(fd, buf, 10);
	printf("\nU: Size of read data : %d\n",retn);
	printf("U: read data : %s\n",buf);

	close(fd);

	return 0;
}
