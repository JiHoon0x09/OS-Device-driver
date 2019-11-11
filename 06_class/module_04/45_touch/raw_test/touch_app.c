  /***************************************
  2  * Filename: sk_app.c
  3  * Title: Skeleton Device Application
  4  * Desc: Implementation of system call
  5  ***************************************/
  #include <stdio.h>
  #include <unistd.h>
  #include <stdlib.h>
  #include <fcntl.h>
 
 int main(void)
 {
     int retn;
     int fd;
     int buf0,buf1;
     short int buf[4] = {0xffff,0xffff,0xffff,0xffff};

     fd = open("/dev/ts0", O_RDWR);
     printf("fd = %d\n", fd);

     if (fd<0) {
         perror("/dev/ts0 error");
         exit(-1);
     }
     else
         printf("TouchScreen  has been detected...\n");

     while(1){
     retn = read(fd, buf, 8);

         printf("\npressure : %d\n", buf[0]);
         printf("\nx_value : %d\n", buf[1]);
         printf("\ny_value : %d\n", buf[2]);
         printf("\nmillisecs : %d\n", buf[3]);
     }
     close(fd);

     return 0;
 }
