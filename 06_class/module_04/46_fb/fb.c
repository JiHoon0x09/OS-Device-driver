/***************************************
 * Filename: fb.c
 * Title: Framebuffer Device Application
 * Desc: /dev/fb0 application
 ***************************************/
#include <stdio.h>
#include <stdlib.h> /* for exit */
#include <unistd.h> /* for open/close .. */
#include <fcntl.h> /* for O_RDONLY */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/fb.h> /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */

#define FBINFO			0
#define	PIXEL			0
#define	COLOR_STYLE1	1
#define	COLOR_STYLE2 	0	

#define FBDEVFILE "/dev/fb0"

#define	XPOS1	0
#define	YPOS1	0
#define	XPOS2	320
#define	YPOS2	240
//#define	XPOS2	480
//#define	YPOS2	272

typedef unsigned char ubyte;

unsigned short makepixel(ubyte r, ubyte g, ubyte b)
{
    return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}

int main()
{
    int fbfd;
    int ret;
    struct fb_var_screeninfo fbvar;
	struct fb_fix_screeninfo fbfix;
    unsigned short pixel;
    unsigned short *pfbdata;
    int offset, t, tt;
    
    fbfd = open(FBDEVFILE, O_RDWR);
    if(fbfd < 0)
    {
        perror("fbdev open");
        exit(1);
    }

#if	FBINFO
    ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar);
    if(ret < 0)
    {
        perror("fbdev ioctl");
        exit(1);
    }
	printf("\nfb_var_screeninfo\n");
	printf("xres = %d, yres = %d\n", fbvar.xres, fbvar.yres);
	printf("xres_virtual = %d, yres_virtual = %d\n", fbvar.xres_virtual, fbvar.yres_virtual);
	printf("xoffset = %d, yoffset = %d\n", fbvar.xoffset, fbvar.yoffset);
	printf("bist_per_pixel = %d\n", fbvar.bits_per_pixel);

    ret = ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix);
    if(ret < 0)
    {
        perror("fbdev ioctl");
        exit(1);
    }
	printf("\nfb_fix_screeninfo\n");
	printf("smem_start = 0x%08X\n", fbfix.smem_start);
	printf("smem_length = %d Kbytes\n", fbfix.smem_len/1024);
#endif	/* FBINOF */
    
#if	PIXEL
    ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar);
    if(ret < 0)
    {
        perror("fbdev ioctl");
        exit(1);
    }
    
    if(fbvar.bits_per_pixel != 16)
    {
        fprintf(stderr, "bpp is not 16\n");
        exit(1);
    }
    
    /* red pixel @ (0,0) */
    if(lseek(fbfd, 0, SEEK_SET) < 0)
    {
        perror("fbdev lseek");
        exit(1);
    }
    pixel = makepixel(255,0,0); /* red pixel */
    write(fbfd, &pixel, 2); /* write 2byte(16bit) */
    
    /* green pixel @ (100,50) */
    offset = 50*fbvar.xres*(16/8)+100*(16/8);
    if(lseek(fbfd, offset, SEEK_SET) < 0)
    {
        perror("fbdev lseek");
        exit(1);
    }
    pixel = makepixel(0,255,0); /* green pixel */
    write(fbfd, &pixel, 2); /* write 2byte(16bit) */
    
    
    /* blue pixel @ (50,100) */
    offset = 100*fbvar.xres*(16/8)+50*(16/8);
    if(lseek(fbfd, offset, SEEK_SET) < 0)
    {
        perror("fbdev lseek");
        exit(1);
    }
    pixel = makepixel(0,0,255); /* blue pixel */
    write(fbfd, &pixel, 2); /* write 2byte(16bit) */
    
    /* white pixel @ (100,100) */
    offset = 100*fbvar.xres*(16/8)+100*(16/8);
    if(lseek(fbfd, offset, SEEK_SET) < 0)
    {
        perror("fbdev lseek");
        exit(1);
    }
    pixel = makepixel(255,255,255); /* white pixel */
    write(fbfd, &pixel, 2); /* write 2byte(16bit) */
#endif	/* PIXEL */

#if	COLOR_STYLE1
	pixel = makepixel(255,0,0);

	for(t=YPOS1; t<YPOS2; t++) {
		for(tt=XPOS1; tt<XPOS2; tt++) {
			offset = t*XPOS2*2+tt*2;
			if(lseek(fbfd, offset, SEEK_SET) < 0) {
				perror("ERROR fbdev lseek");
				exit(1);
			}
			write(fbfd, &pixel, 2);
		}
	}
#endif	/* COLOR_STYLE1 */

#if	COLOR_STYLE2
	pfbdata = (unsigned short *)mmap(0, XPOS2*YPOS2*2, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((unsigned)pfbdata == (unsigned)-1){
		perror("Error fbdev mmap");
		exit(1);
	}

	pixel = makepixel(0,255,0);

	for(t=YPOS1; t<YPOS2; t++) {
		offset = t*XPOS2;
		for(tt=XPOS1; tt<XPOS2; tt++) {
			*(pfbdata + offset + tt) = pixel;
		}
	}

	munmap(pfbdata, XPOS2*YPOS2*2);
#endif	/* COLOR_STYLE2 */
    
    close(fbfd);
    exit(0);
    return 0;
}
