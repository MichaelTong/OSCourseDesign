#include <sys/types.h>

#define BYTE unsigned char

/* 页面大小（字节）*/
#define PAGE_SIZE 4
/* 虚存空间大小（字节） */
#define VIRTUAL_MEMORY_SIZE (64 * 4)
/* 实存空间大小（字节） */
#define ACTUAL_MEMORY_SIZE (32 * 4)
/* 总虚页数 */
#define PAGE_SUM (VIRTUAL_MEMORY_SIZE / PAGE_SIZE)
/* 总物理块数 */
#define BLOCK_SUM (ACTUAL_MEMORY_SIZE / PAGE_SIZE)
/* 访存请求类型 */
typedef enum {
	REQUEST_READ,
	REQUEST_WRITE,
	REQUEST_EXECUTE,
	REQUEST_OVER
} MemoryAccessRequestType;

/* 访存请求 */
typedef struct
{
	pid_t pid;
	MemoryAccessRequestType reqType; //访存请求类型
	unsigned long virAddr; //虚地址
	BYTE value; //写请求的值
} MemoryAccessRequest, *Ptr_MemoryAccessRequest;
