#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<linux/ioctl.h>
#include"sk.h"

int main(void){
	int retn;
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

	while(1){
		ioctl(fd, SK_LED_ON, flag);
		getchar();
		ioctl(fd, SK_LED_OFF, flag);
		getchar();
	}
	return 0;
}
