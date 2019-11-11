/*****************************************************
 * vdisk.c
 *
 * 실질적인 디바이스 드라이버는 아님.
 * 단지 메모리를 할당하고 해제할 뿐.
 * 그리고 해당 메모리를 외부 모듈에서 참조할 수
 * 있도록 지원할 뿐.
 * 일종의 가상 하드 디스크
 *****************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/vmalloc.h>

#define   VHDD_MAX_DEVICE           2
#define   VHDD_DEVICE_SIZE          (4*1024*1024)	// 4MB

char *vdisk[VHDD_MAX_DEVICE] = {NULL,};

int vdisk_init(void)
{
    int result;

	// 사용할 메모리 공간 할당
    vdisk[0] = vmalloc(VHDD_DEVICE_SIZE);
    vdisk[1] = vmalloc(VHDD_DEVICE_SIZE);
    
	// 메모리 0으로 초기화
    memset( vdisk[0], 0, VHDD_DEVICE_SIZE );
    memset( vdisk[1], 0, VHDD_DEVICE_SIZE );

    return 0;
}

void vdisk_exit(void)
{
    vfree( vdisk[0]);
    vfree( vdisk[1]);
}

/**************************************
 * 할당된 메모리를 외부에서 사용할 수
 * 있도록 외부 심볼로 선언
 **************************************/
EXPORT_SYMBOL(vdisk);

module_init(vdisk_init);
module_exit(vdisk_exit);

MODULE_LICENSE("Dual BSD/GPL");
