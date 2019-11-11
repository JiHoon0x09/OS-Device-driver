/***************************************
 * Filename: sk_app.c
 * Title: Skeleton Device Application
 * Desc: Implementation of system call
 ***************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void)
{
    int retn;
    int fdp, fdc;
	int pid, i;

    char buf[100] = "this is fucntion..\n";
    
    fdp = open("/dev/sleepy", O_RDWR);
    if (fdp < 0) {
        perror("/dev/sleepy open error...!!");
        exit(-1);
    }
    //else
    //    printf("Sleepy has been detected...(parent)\n");
    
	pid = fork();
	if(pid < 0) {
		perror("fork error!!!");
		exit(-1);
	}
	else if(pid == 0) {	// child
		for(i=0;i<100000000;i++);
    	fdc = open("/dev/sleepy", O_RDWR);
    	if (fdc < 0) {
        	perror("/dev/sleepy open error...!!");
        	exit(-1);
    	}
    //	else
    //    	printf("Sleepy has been detected...(child)\n");

		retn = write(fdc, buf, 20);

		close(fdc);

		exit(0);
	}
	else {				// parent
    	retn = read(fdp, buf, 20);
		wait(NULL);
    	printf("\ndata : %s\n", buf);
    	
		close(fdp);

		exit(0);
	}

}
