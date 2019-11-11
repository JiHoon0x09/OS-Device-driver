#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<linux/ioctl.h>

int main(void){
	int retn;
	char buf[100] = {0};
	int fd;
	int flag = 0;

	fd = open("/dev/sk",O_RDWR);
	printf("fd = %d \n\n",fd);

	if(fd < 0){
		perror("/dev/sk ERROR!!\n");
		exit(-1);
	}
	else
		printf("sk module has been detected...\n");

	getchar();

	ioctl(fd,'A',flag); getchar();
	ioctl(fd,'B',flag); getchar();
	ioctl(fd,'C',flag); getchar();
	ioctl(fd,'D',flag); getchar();
	ioctl(fd,'E',flag); getchar();
	ioctl(fd,'F',flag); getchar();
	ioctl(fd,'G',flag); getchar();
	ioctl(fd,'H',flag); getchar();
	close(fd);

	return 0;
}
