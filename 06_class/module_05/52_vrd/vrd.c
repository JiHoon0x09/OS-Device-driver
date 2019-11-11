/***************************************
 * Filename: vrd.c
 * Title: Virtual Ram Disk module
 * Desc: Block Device Example
 ***************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/vmalloc.h>
#include <linux/hdreg.h>  
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <asm/uaccess.h>

#define   VRD_DEV_NAME            "vrd"
#define   VRD_DEV_MAJOR            240

#define   VRD_MAX_DEVICE           2

#define   VRD_SECTOR_SIZE         512
#define   VRD_SIZE                (4*1024*1024)
#define   VRD_SECTOR_TOTAL        (VRD_SIZE/VRD_SECTOR_SIZE)

typedef struct 
{
    unsigned char        *data;
    struct request_queue *queue;
    struct gendisk       *gd;
} vrd_device;

static char                 *vdisk[VRD_MAX_DEVICE];
static vrd_device            device[VRD_MAX_DEVICE];

/***************************************************************
 * Request queue가 없는 Request 함수
 * 블록 디바이스의 실질적인 처리가 이루어짐.
 * 전달된 request_queue_t 구조체의 q 매개변수는 사용되지 않음.
 * 실질적으로 사용되는 것은 bio 구조체인 bio이다.
 ***************************************************************/
static int vrd_make_request(request_queue_t *q, struct bio *bio)
{
    vrd_device     *pdevice;
    char           *pVHDDData;
    char           *pBuffer; 
    struct bio_vec *bvec;
    int             i;

	/*****************************************************************
	 * 요구된 섹터의 크기와 위치가 디바이스의 범위를 넘지 않는지 검사
	 * bi_sector : 요구한 시작 섹터 번호
	 * bi_size : 요구되는 섹터 전체 크기(바이트 단위)
	 * VRD_SIZE : 디바이스 전체 크기(바이트 단위)
	 *****************************************************************/
    if( ( (bio->bi_sector*VRD_SECTOR_SIZE) + bio->bi_size ) > VRD_SIZE ) goto fail;  
    
	// gendisk의 private_data에 할당했던 vrd_device 구조체의 주소값을 얻어온다.
    pdevice = (vrd_device *) bio->bi_bdev->bd_disk->private_data;

	/*******************************************************************
	 * pdevice->data의 주소를 이용 (bio->bi_sector*VRD_SECTOR_SIZE)에
	 * 해당하는 메모리 주소를 얻어옴.
	 * bi_sector(섹터 시작 번호) * 512
	 *******************************************************************/
    pVHDDData  = pdevice->data + (bio->bi_sector*VRD_SECTOR_SIZE);
    
	/*******************************************************************
	 * bio 매개변수는 처리 요구에 대한 내용을 담고 있는 bio_vec 구조체를
	 * 여러개 포함하고 있다. 각 bio_vec 구조체 벡터를 차례로 처리
	 * bvec : 내부적으로 각 bio_vec을 처리하기 위해 넘겨주는 값
	 * i : 내부적으로 for loop를 수행하기 위한 값(변경 X)
	 *******************************************************************/
    bio_for_each_segment(bvec, bio, i)
    {
		// buffer 주소 계산
        pBuffer = kmap(bvec->bv_page) + bvec->bv_offset;
		// request의 종류 판별, bio->bi_rw 체크
        switch(bio_data_dir(bio)) 
        {
        case READA :         
        case READ  : memcpy(pBuffer, pVHDDData, bvec->bv_len);
					printk("Read Data\n");
                     break; 
        case WRITE : memcpy(pVHDDData, pBuffer, bvec->bv_len); 
					printk("Write Data\n");
                     break;
		// 이외의 값은 오류이므로 매핑을 해제하고 에러처리
        default    : kunmap(bvec->bv_page);
                     goto fail;
        }
        kunmap(bvec->bv_page);	// 매핑 해제
        pVHDDData += bvec->bv_len;	// 다음 작업을 위해 bv_len만큼 주소 증가시킴.
    }
    
	// 모든 처리가 끝나면 처리가 종료되었음을 표시
    bio_endio(bio, bio->bi_size, 0);
    return 0;
fail:	// 에러처리
    bio_io_error(bio, bio->bi_size);
    return 0;
} 

int vrd_open(struct inode *inode, struct file *filp)
{
	/***************************************************************
	 * 디바이스가 마운트되면 호출. 특별히 처리할 작업이 없음.
	 * 만약 제거할 수 있는 디바이스나 완전한 디바이스 드라이버라면 
	 * 디바이스의 사용 수를 자체적으로 처리해야 함.
	 ***************************************************************/
    return 0;
}

int vrd_release (struct inode *inode, struct file *filp)
{
	/**************************************************************
	 * 디바이스가 언마운트되면 호출. 특별히 처리할 작업이 없음.
	 * 만약 제거할 수 있는 디바이스나 완전한 디바이스 드라이버라면 
	 * 디바이스의 사용 수를 자체적으로 처리해야 함.
	 ***************************************************************/
    return 0;
}

int vrd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	/**************************************************************
	 * 간단한 램디스크이므로 ioctl 명령에 대해 모두 -ENOTTY 반환
	 * 커널 2.6에서의 ioctl() 함수는 대부분 커널 내부에서 알아서
	 * 처리함.
	 **************************************************************/
    return -ENOTTY;
}

static struct block_device_operations vrd_fops =
{
    .owner   = THIS_MODULE,    
    .open    = vrd_open,
    .release = vrd_release,  
    .ioctl   = vrd_ioctl,
};

int vrd_init(void)
{
    int lp;
    
    vdisk[0] = vmalloc( VRD_SIZE );
    vdisk[1] = vmalloc( VRD_SIZE );

    register_blkdev(VRD_DEV_MAJOR, VRD_DEV_NAME);
    
    for( lp = 0; lp < VRD_MAX_DEVICE; lp++ )
    {

        device[lp].data  = vdisk[lp];
        device[lp].gd    = alloc_disk(1);	// gendisk 구조체를 하나씩 동적 할당
        device[lp].queue = blk_alloc_queue(GFP_KERNEL);	// Request queue 할당
        blk_queue_make_request(device[lp].queue, &vrd_make_request);	// make_request 함수 등록

        device[lp].gd->major         = VRD_DEV_MAJOR;	// Major No' 할당
        device[lp].gd->first_minor   = lp;				// Minor No' 할당(increment
        device[lp].gd->fops          = &vrd_fops;		// block device 메소드 구조체 포인터
        device[lp].gd->queue         = device[lp].queue;// I/O 요청을 다루기 위해 커널이 사용하는 구조체
        device[lp].gd->private_data  = &device[lp];		// 내부적으로 사용할 자료 포인터
		// procfs과 sysfs에 파티션 및 블록 디바이스를 표현(vrda, vrdb)
        sprintf(device[lp].gd->disk_name,  "vrd%c" , 'a'+lp);
        set_capacity(device[lp].gd, VRD_SECTOR_TOTAL );	// 각 블록 디바이스의 총 sector 갯수를 지정
		
        add_disk(device[lp].gd);	// 디바이스 활성화
        
    }
    return 0;
    
}

void vrd_exit(void)
{
    int lp;

    for(lp = 0; lp < VRD_MAX_DEVICE; lp++)
    {
		del_gendisk(device[lp].gd);	// gendisk 구조체 반환
		put_disk(device[lp].gd);
    }
    unregister_blkdev( VRD_DEV_MAJOR, VRD_DEV_NAME );

    vfree( vdisk[0]);
    vfree( vdisk[1]);
}

module_init(vrd_init);
module_exit(vrd_exit);

MODULE_LICENSE("Dual BSD/GPL");
