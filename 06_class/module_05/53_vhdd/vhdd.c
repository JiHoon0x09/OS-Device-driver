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

#define   VHDD_DEV_NAME            "vhdd"
#define   VHDD_DEV_MAJOR            241
#define   VHDD_MAX_DEVICE           2
#define   VHDD_MAX_PARTITONS        16

#define   VHDD_SECTOR_SIZE         512
#define   VHDD_SECTORS             16	// 한 실린더 내의 섹터 수
#define   VHDD_HEADS               2
#define   VHDD_CYLINDERS           256

#define   VHDD_SECTOR_TOTAL        (VHDD_SECTORS*VHDD_HEADS*VHDD_CYLINDERS)
#define   VHDD_SIZE                (VHDD_SECTOR_SIZE*VHDD_SECTOR_TOTAL)

// 디바이스 데이터 필드 정의
typedef struct 
{
    unsigned char        *data;
    struct request_queue *queue;
    spinlock_t            lock;		// request() 함수는 원자적이야 하므로
    struct gendisk       *gd;
} vhdd_device;

// vdisk의 외부 변수를 참조하기 위한 변수 선언
extern char                 *vdisk[VHDD_MAX_DEVICE];

// vhdd_device를 할당하고 관리하기 위해 device 배열 선언
static vhdd_device           device[VHDD_MAX_DEVICE];
/***********************************************************
 * 하드디스크가 동작하는 것처럼 보이기 위해 커널 타이머를
 * 이용하여 인터럽트를 구현
 * 커널 타이머를 이용하기 위한 timer_lis 구조체 변수 선언
 ***********************************************************/
static struct timer_list     vhdd_timer; 
/***********************************************************
 * request 함수의 재진입을 처리하기 위한 변수 선언
 * 이 값이 0이 아니면 vhdd_request() 함수는 처리 중
 ***********************************************************/
static int                   vhdd_busy = 0;

/***************************************************************
 * 블록 디바이스의 실질적인 처리가 이루어짐.
 * 커널이 이 함수를 호출해 블록 디바이스 드라이버에 처리를 요구
 * 전달된 매개변수를 이용해 처리
 ***************************************************************/
static void vhdd_request(request_queue_t * q)
{
    vhdd_device     *pdevce;
        
    struct request  *vhdd_req;	
    char            *pData;
    int              size;
    
	/***********************************************************
	 * 현재 블록 디바이스가 request를 처리 중인지 검사
	 * 인터럽트 처리를 수행하기 때문에 처리중에 
	 * vhdd_request 함수가 호출될 수 있기 때문
	 ***********************************************************/
    if( vhdd_busy ) return;	
    
	/***********************************************************
	 * 대기 중인 요구가 모두 처리될 때까지 반복적으로 처리
	 ***********************************************************/
    while(1) 
    {
		/***************************************************************
		 * 처리 요구를 획득
		 * NULL을 반환하면 더이상 처리할 요구가 없는 것이므로 함수 종료
		 ***************************************************************/
		vhdd_req = elv_next_request(q);
		if (!vhdd_req) return;
	
		/***************************************************************
		 * 요구 위치에 해당하는 메모리 주소를 구하기 위해
		 * gendisk의 private_data에 할당했던 vhdd_device 관리 구조체의
		 * 주소를 얻는다.
		 ***************************************************************/
		pdevce = (vhdd_device *) vhdd_req->rq_disk->private_data;
	
		/***************************************************************
		 * 처리해야 할 시작 섹터는 vhdd_req->sector에 전달
		 * pdevce->data에 (vhdd_req->sector)*VHDD_SECTOR_SIZE를 더해
		 * 해당하는 메모리 주소를 얻는다.
		 * vhdd_req(request 구조체)->sector : 디바이스의 시작 섹터 번호
		 * VHDD_SECTOR_SIZE : 한 섹터의 크기(바이트 단위)
		 * 시작 섹터 번호 * 512 = 해당 섹터 시작 주소
		 ***************************************************************/
		pData = pdevce->data + (vhdd_req->sector)*VHDD_SECTOR_SIZE;

		/***************************************************************
		 * 요구된 시작 섹터와 크기가 블록 디바이스 범위를 넘는지 검사
		 * 처리 요구 시작 섹터 vhdd_req->sector와 현재 처리해야할 섹터의
		 * 갯수를 가진 vhdd_req->current_nr_sectors를 더해 현재 블록
		 * 디바이스의 총 섹터 갯수(get_capacity)를 넘는지 검사
		 ***************************************************************/
		if (vhdd_req->sector + vhdd_req->current_nr_sectors > get_capacity(vhdd_req->rq_disk))
		{
			/********************************************************
			 * 커널에 에러 통보하고, 다음 요구를 처리
			 ********************************************************/
            end_request(vhdd_req, 0);
            continue;
        }
        
		/***************************************************************
		 * 현재 처리해야할 섹터의 갯수를 이용하여 처리해야할 데이터
		 * 크기를 구함.
		 * 처리해야할 섹터의 갯수 * 512 = 처리 데이터 사이즈
		 ***************************************************************/
        size = vhdd_req->current_nr_sectors * VHDD_SECTOR_SIZE;

		/***************************************************************
		 * request의 종류 판별, request->flags 체크 
		 ***************************************************************/
        switch(rq_data_dir(vhdd_req)) 
        {
        	case READ  : memcpy(vhdd_req->buffer, pData, size); break; 
        	case WRITE : memcpy(pData, vhdd_req->buffer, size); break;
			/*****************************************************
			 * 이외의 값은 오류이므로 
			 * 커널에 에러를 통보하고, 다음 요구를 처리
			 *****************************************************/
        	default    : end_request(vhdd_req,0);	continue;
        }

		/***************************************************************
		 * 일반적인 하드디스크는 읽거나 쓸때 읽고자 하는 섹터 정보를 전송,
		 * 인터럽트가 발생하면 데이터를 읽거나 쓴다.
		 * 본 예제는 물리 디스크가 없으므로 이를 흉내내기 위해
		 * 커널 타이머를 이용
		 ***************************************************************/
        vhdd_timer.expires = get_jiffies_64() + 2;	// 타이머 발생 시점
        vhdd_timer.data    = (unsigned long) pdevce;// 인터럽트 핸들러로 넘겨줄 데이터
        add_timer(&vhdd_timer);	// 타이머 등록
        vhdd_busy = 1;	// busy 상태로 만듦.
        return;
    }
}

/***************************************************************
 * 등록된 커널 타이머에 의해 일정 시간 후 하드 디스크가 처리를
 * 끝내고 인터럽트가 발생한 것처럼 vhdd_interrupt를 호출
 ***************************************************************/
void vhdd_interrupt( unsigned long data )
{
    vhdd_device     *pdevce;
    struct request  *vhdd_req;	
    
    pdevce   = (vhdd_device *) data;
    vhdd_req = elv_next_request(pdevce->queue);

	/*****************************************************************
	 * 처리가 끝났음을 커널에 알림
	 * end_request(1) : 정상적인 처리 완료
	 * ebd_request(0) : 오류 처리
	 *****************************************************************/
    end_request(vhdd_req, 1);
	/*****************************************************************
	 * 처리가 끝나 디바이스 드라이버 작업이 멈추었음을 나타내기 위해
	 * vhdd_busy 값을 0으로 조정
	 *****************************************************************/
    vhdd_busy = 0;	// non-busy 상태
    vhdd_request(pdevce->queue);	// 다음 요구를 처리
}

int vhdd_open(struct inode *inode, struct file *filp)
{
	/*****************************************************************
	 * 디바이스가 마운트되면 호출. 특별히 처리할 작업이 없음.
	 * 만약 제거할 수 있는 디바이스나 완전한 디바이스 드라이버라면
	 * 디바이스의 사용 수를 자체적으로 처리해야 함.
	 *****************************************************************/
    return 0;
}

int vhdd_release (struct inode *inode, struct file *filp)
{
	/*****************************************************************
	 * 디바이스가 언마운트되면 호출. 특별히 처리할 작업이 없음.
	 * 만약 제거할 수 있는 디바이스나 완전한 디바이스 드라이버라면
	 * 디바이스의 사용 수를 자체적으로 처리해야 함.
	 *****************************************************************/
    return 0;
}

/**************************************************************
 * 디스크 구조를 전달하는 HDIO_GETGEO만 처리
 * 나머지는 모두 -ENOTTY를 반환
 **************************************************************/
int vhdd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err;
    struct hd_geometry geo;

    switch(cmd) 
    {
	/*********************************************************
	 * HDIO_GETGEO는 fdisk와 같은 파티션 처리 프로그램에서
	 * 디바이스의 디스크 구조 정보를 얻는 명령
	 * *******************************************************/
	case HDIO_GETGEO:
        err = ! access_ok(VERIFY_WRITE, arg, sizeof(geo));
        if (err) return -EFAULT;

		/*************************************************************
		 * hd_geometry 구조체 변수에 실린더 수, 헤드 수, 섹터 수 및
		 * 실질적인 섹터의 시작값을 전달
		 *************************************************************/
        geo.cylinders = VHDD_CYLINDERS;
		geo.heads     = VHDD_HEADS;
		geo.sectors   = VHDD_SECTORS;
		geo.start = get_start_sect(inode->i_bdev);
		if (copy_to_user((void *) arg, &geo, sizeof(geo))) return -EFAULT;
        return 0;
    }

    return -ENOTTY;
}

static struct block_device_operations vhdd_fops =
{
    .owner   = THIS_MODULE,
    .open    = vhdd_open,
    .release = vhdd_release,  
    .ioctl   = vhdd_ioctl,
};

int vhdd_init(void)
{
    int lp;

    init_timer( &(vhdd_timer) );	// 커널 타이머 초기화
    vhdd_timer.function = vhdd_interrupt;	// 커널 타이머 인터럽트 핸들러 할당

    register_blkdev(VHDD_DEV_MAJOR, VHDD_DEV_NAME);
    
    for( lp = 0; lp < VHDD_MAX_DEVICE; lp++ ) // 디바이스 수만큼 반복
    {
		/*****************************************************************
		 * vdisk에서 얻은 메모리 주소를 request 함수에서 사용할 수 있도록
		 * 설정
		 *****************************************************************/
        device[lp].data = vdisk[lp];
		/*****************************************************************
		 * gendisk 구조체를 파티션 수만큼 할당
		 * 커널 내부에서는 이 값이 2 이상이면 파티션을 존재하는 것으로
		 * 판단하고, 파티션 처리를 수행
		 *****************************************************************/
        device[lp].gd   = alloc_disk(VHDD_MAX_PARTITONS);
        spin_lock_init(&device[lp].lock);	// lock 획득
		/*****************************************************************
		 * 커널과 블록 디바이스의 실질적은 통로인 vhdd_request() 함수 등록
		 *****************************************************************/
        device[lp].queue = blk_init_queue(vhdd_request, &device[lp].lock);

        device[lp].gd->major         = VHDD_DEV_MAJOR;	// Major No 할당
        device[lp].gd->first_minor   = lp*VHDD_MAX_PARTITONS; // 각 디바이스의 처음 시작 Minor No 할당
        device[lp].gd->fops          = &vhdd_fops;	// operation 등록
        device[lp].gd->queue         = device[lp].queue;
        device[lp].gd->private_data  = &device[lp];	// 내부적으로 사용할 자료 포인터
		// procfs과 sysfs에 파티션 및 블록 디바이스를 표현(vhdda, vhddb)
        sprintf(device[lp].gd->disk_name,  "vhdd%c" , 'a'+lp);
        set_capacity(device[lp].gd, VHDD_SECTOR_TOTAL );	// 각 블록 디바이스의 총 섹터
		
        add_disk(device[lp].gd);	// 디아이스 활성화
        
    }
    return 0;
}

void vhdd_exit(void)
{
    int lp;

	// 등록된 디바이스 수만큼 반복
    for(lp = 0; lp < VHDD_MAX_DEVICE; lp++)
    {
		// 등록된 gendisk 구조체를 커널에서 제거
		del_gendisk(device[lp].gd);
		// 할당된 gendisk 구조체 메모리를 해제
		put_disk(device[lp].gd);
	
		// request_queue 제거
		blk_cleanup_queue(device[lp].queue);
    }
	// 블록 디바이스 제거
    unregister_blkdev( VHDD_DEV_MAJOR, VHDD_DEV_NAME );
}

module_init(vhdd_init);
module_exit(vhdd_exit);

MODULE_LICENSE("Dual BSD/GPL");
