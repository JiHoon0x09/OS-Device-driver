/*******************************************
 * Filename: vrd_app.c
 * Title: Virtual Ram Disk
 * Desc: Block Device Application Example
 *******************************************/

 #include <stdio.h>
 #include <string.h>
 #include <fcntl.h>
 #include <linux/fs.h>

 int main(int argc, char *argv[])
 {
 	int fd, n;
	int sector_size = 0;
	char buffer[100];
	int cnt;

	//fd = open("/dev/vrd0", O_RDONLY, 0);
	fd = open("/dev/vrd0", O_RDWR, 0);
	//printf("select number(1. Total size | 2. Sector size) : ");
	printf("1. Total size\n");
	printf("2. Sector size\n");
	printf("3. Write data\n");
	printf("4. Read data\n");
	printf("select number : ");
	scanf("%d", &n);

	switch(n)
	{
		case 1:
			ioctl(fd, BLKGETSIZE, &sector_size);
			printf("VRD_SECTOR_TOTAL : %d\n", sector_size);
			return 0;
		case 2:
			//ioctl(fd, 0, &sector_size);
			ioctl(fd, BLKSSZGET, &sector_size);
			printf("VRD_SECTOR_SIZE : %d\n", sector_size);
			return 0;
		case 3:
			sprintf(buffer, "vrd Write Data-kkk");
			printf("buffer len = %d\n", strlen(buffer));
			write(fd, buffer, strlen(buffer));
			return 0;
		case 4:
			cnt = read(fd, buffer, 100);
			printf("Read string number : %d\n", cnt);
			printf("Read Data : %s\n", buffer);
			return 0;
		default:
			return 0;
	}
 }
