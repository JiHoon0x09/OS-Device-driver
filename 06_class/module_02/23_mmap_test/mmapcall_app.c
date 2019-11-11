/****************************************
 *	Filename: mmapcall_app.c
 *	Title: MMAP Test Application
 *	Desc:
 ****************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define	DEVICE_FILENAME	"/dev/mmapcall"

#define	MMAP_SIZE		0x1000			/* 4096 byte */

int main(void)
{
	int		dev;
	int		loop;
	char	*ptrdata;

	dev = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);

	if(dev >= 0)
	{
		/* 메모리 영역 맵핑 시도 */
		ptrdata = (char *)mmap(0x12345000,	/* Start Address */
		//ptrdata = (char *)mmap(0,	/* Start Address */
						MMAP_SIZE,
						PROT_READ|PROT_WRITE,
						MAP_SHARED,			/* 여러 프로세스와 공유 */
						dev,
						0x87654000);		/* Offset Address */

		/* 메모리 영역 해제 */
		munmap(ptrdata, MMAP_SIZE);

		close(dev);
	}

	return 0;
}
