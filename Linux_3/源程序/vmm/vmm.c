#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "vmm.h"

/* 程序数量 */
int pronum,curpro;
/* 各页使用次数 */
unsigned int pagecount[PAGE_SUM];
int useflag[PAGE_SUM];
/* 多道程序页表 */
Ptr_ProPage ptr_propage,head,tail;
/* 实存空间 */
BYTE actMem[ACTUAL_MEMORY_SIZE];
/* 用文件模拟辅存空间 */
FILE *ptr_auxMem;
/* 物理块使用标识 */
BOOL blockStatus[BLOCK_SUM];
/* 访存请求 */
Ptr_MemoryAccessRequest ptr_memAccReq;
/* 快表 */
TLBItem TLB[TLB_SUM];

int fifo;

Ptr_ProPage init_page(int pro)
{
    Ptr_ProPage newPage=(Ptr_ProPage)malloc(sizeof(ProPage));
    Ptr_PageTableItem pageTable=newPage->pageTable;
    srandom(time(NULL));
    int i,j;

	for (i = 0; i < PAGE_SUM; i++)
	{
		pageTable[i].pageNum = i;
		pageTable[i].filled = FALSE;
		pageTable[i].edited = FALSE;
		pageTable[i].count = 0;
		pageTable[i].useflag=0;
		/* 使用随机数设置该页的保护类型 */
		switch (random() % 7)
		{
			case 0:
			{
				pageTable[i].proType = READABLE;
				break;
			}
			case 1:
			{
				pageTable[i].proType = WRITABLE;
				break;
			}
			case 2:
			{
				pageTable[i].proType = EXECUTABLE;
				break;
			}
			case 3:
			{
				pageTable[i].proType = READABLE | WRITABLE;
				break;
			}
			case 4:
			{
				pageTable[i].proType = READABLE | EXECUTABLE;
				break;
			}
			case 5:
			{
				pageTable[i].proType = WRITABLE | EXECUTABLE;
				break;
			}
			case 6:
			{
				pageTable[i].proType = READABLE | WRITABLE | EXECUTABLE;
				break;
			}
			default:
				break;
		}
		/* 设置该页对应的辅存地址 */
		pageTable[i].auxAddr = i * PAGE_SIZE * 2;
	}
	for (j = 0; j < BLOCK_SUM; j++)
	{
		/* 随机选择一些物理块进行页面装入 */
		if (random() % 2 == 0 && blockStatus[j]==FALSE)
		{
			do_page_in(&pageTable[j], j);
			pageTable[j].blockNum = j;
			pageTable[j].filled = TRUE;
			blockStatus[j] = TRUE;
		}
	}
	newPage->pro = pro;
	newPage->next = NULL;
	//add page to propage link
	if(head == NULL)
	{
	    head = newPage;
	    tail=newPage;
	    //tail=ptr_propage;
	}
	else
	{
	    tail->next = newPage;
	    tail = tail->next;
	}
	return newPage;
}
/* 初始化环境 */
void do_init()
{
	struct stat statbuf;

    if(stat("/tmp/vmm.temp",&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove("/tmp/vmm.temp")<0)
			printf("remove failed\n");
	}

	if(mkfifo("/tmp/vmm.temp",0666)<0)
		printf("mkfifo failed");
	/* 在非阻塞模式下打开FIFO */
	if((fifo=open("/tmp/vmm.temp",O_RDONLY|O_NONBLOCK))<0)
		printf("open fifo failed");


}

Ptr_TLBItem TLBLookUp(unsigned int pageNum,pid_t pro)
{
    int i=0;
    for(;i<TLB_SUM;i++)
        if(TLB[i].feature==1&&TLB[i].pageNum==pageNum&&TLB[i].pro==pro)
        {
            TLB[i].count++;
            return &TLB[i];
        }
    return 0;
}
Ptr_TLBItem do_TLB_in(unsigned int pageNum,unsigned int blockNum,pid_t pro,int proType)
{
    int i,min,j;
    for(i=0;i<TLB_SUM;i++)
    {
        if(TLB[i].feature==0)
            break;
    }
    if(i==TLB_SUM)
    {
        for(j=0;j<TLB_SUM;j++)
        {
            if(j==0)
            {
                i=j;
                min=TLB[j].count;
            }
            else if(min>TLB[j].count)
            {
                i=j;
                min=TLB[j].count;
            }
        }
    }
    TLB[i].pro=pro;
    TLB[i].count=1;
    TLB[i].feature=1;
    TLB[i].pageNum=pageNum;
    TLB[i].blockNum=blockNum;
    TLB[i].proType=proType;
    return &TLB[i];
}
void do_delete(pid_t pro)
{
    int i;
    Ptr_ProPage p,q;
    for(i=0;i<TLB_SUM;i++)
    {
        if(TLB[i].pro==pro)
        {
            TLB[i].feature=FALSE;
        }
    }
    p=head;
    if(p->pro==pro)
    {
        head=p->next;
        free(p);
        p=NULL;
        return;
    }
    for(q=head,p=head->next;p!=NULL;p=p->next,q=q->next)
    {
        if(p->pro==pro)
        {
            q->next=p->next;
            int i;
            for(i=0;i<PAGE_SUM;i++)
            {
                if(p->pageTable[i].filled==TRUE)
                blockStatus[p->pageTable[i].blockNum]=FALSE;
            }
            free(p);
            p=NULL;
            return;
        }
    }
}

/* 响应请求 */
void do_response(pid_t pro)
{
    Ptr_ProPage p;
	Ptr_PageTableItem ptr_pageTabIt;
	Ptr_TLBItem ptr_TLBIt;
	BYTE proType;
	int pageNum, offAddr;
	int actAddr;
    if(ptr_memAccReq->reqType==REQUEST_OVER)
    {
        do_delete(pro);
        printf("Process Complete, delete page table.\n");
        return;
    }
	/* 检查地址是否越界 */
	if (ptr_memAccReq->virAddr < 0 || ptr_memAccReq->virAddr >= VIRTUAL_MEMORY_SIZE)
	{
		do_error(ERROR_OVER_BOUNDARY);
		return;
	}

	/* 计算页号和页内偏移值 */
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	printf("Proc[%d] Page No. : %d[0x%X]\tOffset : %d[0x%X]\n", pro,pageNum,pageNum, offAddr,offAddr);

    ptr_TLBIt=TLBLookUp(pageNum,pro);
    if(ptr_TLBIt)
    {
        printf("Found page in TLB.\n");
        actAddr = ptr_TLBIt->blockNum * PAGE_SIZE + offAddr;
        printf("Real Address : %u\n", actAddr);
    }
    else{
        p = get_ptr_propage(pro);
        if(p==NULL)
        {
            printf("Load the page for process[%d]...\n",pro);
            p=init_page(pro);
        }
        /* 获取对应页表项 */
        ptr_pageTabIt = &p->pageTable[pageNum];
        /* 根据特征位决定是否产生缺页中断 */
        if (!ptr_pageTabIt->filled)
        {
            if(do_page_fault(ptr_pageTabIt,pro)==-1)
                return;
        }
        /* 装入快表 */
        ptr_TLBIt=do_TLB_in(pageNum,ptr_pageTabIt->blockNum,pro,ptr_pageTabIt->proType);
        printf("Load page to TLB.\n");
    	actAddr = ptr_pageTabIt->blockNum * PAGE_SIZE + offAddr;
    	printf("Real Address : %u[0x%X]\n", actAddr,actAddr);
	    ptr_pageTabIt->useflag=1;
        ptr_pageTabIt->count++;
        if(ptr_pageTabIt->proType==WRITABLE&&ptr_memAccReq->reqType==REQUEST_WRITE)
			ptr_pageTabIt->edited = TRUE;
    }
    proType=ptr_TLBIt->proType;
	/* 检查页面访问权限并处理访存请求 */
	switch (ptr_memAccReq->reqType)
	{
		case REQUEST_READ: //读请求
		{
			if (!(proType & READABLE)) //页面不可读
			{
				do_error(ERROR_READ_DENY);
				return;
			}
			/* 读取实存中的内容 */
			printf("Read Success : The Value is %02X\n", actMem[actAddr]);
			break;
		}
		case REQUEST_WRITE: //写请求
		{
			if (!(proType & WRITABLE)) //页面不可写
			{
				do_error(ERROR_WRITE_DENY);
				return;
			}
			/* 向实存中写入请求的内容 */
			actMem[actAddr] = ptr_memAccReq->value;
			printf("Write Success\n");
			break;
		}
		case REQUEST_EXECUTE: //执行请求
		{
			ptr_pageTabIt->count++;
			if (!(proType & EXECUTABLE)) //页面不可执行
			{
				do_error(ERROR_EXECUTE_DENY);
				return;
			}
			printf("Execute Success\n");
			break;
		}
		case REQUEST_OVER:
		{
            do_delete(pro);
            printf("Process Complete, delete page table.\n");
            break;
		}
		default: //非法请求类型
		{
			do_error(ERROR_INVALID_REQUEST);
			return;
		}
	}
}

/* 处理缺页中断 */
int do_page_fault(Ptr_PageTableItem ptr_pageTabIt,pid_t pro)
{
	unsigned int i;
	printf("Page fault, begin paging...\n");
	for (i = 0; i < BLOCK_SUM; i++)
	{
		if (!blockStatus[i])
		{
			/* 读辅存内容，写入到实存 */
			do_page_in(ptr_pageTabIt, i);

			/* 更新页表内容 */
			ptr_pageTabIt->blockNum = i;
			ptr_pageTabIt->filled = TRUE;
			ptr_pageTabIt->edited = FALSE;
			ptr_pageTabIt->count = 0;

			blockStatus[i] = TRUE;
			return 0;
		}
	}
	/* 没有空闲物理块，进行页面替换 */
	return do_LRU(ptr_pageTabIt,pro);
}

/* 根据LRU算法进行页面替换 */
int do_LRU(Ptr_PageTableItem ptr_pageTabIt,pid_t pro)
{
    Ptr_ProPage p;
	unsigned int i, min, page;
	printf("No free blocks, begin LRU...\n");

    for(i=0,min=0xffffffff,page=-1;i<PAGE_SUM;i++)
    {
        if(!TLBLookUp(i,pro)&&ptr_pageTabIt[i].filled && ptr_pageTabIt[i].count<min)
        {
            page=i;
            min=ptr_pageTabIt[i].count;
        }
    }
    if(page==-1)
    {
        printf("Paging Failed. Memory used out.\n");
        return -1;
    }
	printf("Replace Page No. %u\n", page);
	p=get_ptr_propage(pro);
	if (p->pageTable[page].edited)
	{
		/* 页面内容有修改，需要写回至辅存 */
		printf("The page has been modified, write back\n");
		do_page_out(&p->pageTable[page]);
	}
/*	for(p=head;p!=NULL;p=p->next)
	{
	    p->pageTable[page].filled = FALSE;
	    p->pageTable[page].edited = FALSE;
	}*/

//    p=get_ptr_propage(curpro);
	/* 读辅存内容，写入到实存 */
	do_page_in(ptr_pageTabIt, p->pageTable[page].blockNum);

	/* 更新页表内容 */
	ptr_pageTabIt->blockNum = p->pageTable[page].blockNum;
	ptr_pageTabIt->filled = TRUE;
	ptr_pageTabIt->edited = FALSE;
	ptr_pageTabIt->count = 0;
	//printf("Paging Success : SecMem %lu-->>Block %u\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
	return 0;
}

/* 将辅存内容写入实存 */
void do_page_in(Ptr_PageTableItem ptr_pageTabIt, unsigned int blockNum)
{
	unsigned int readNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((readNum = fread(actMem + blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: blockNum=%u\treadNum=%u\n", blockNum, readNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_READ_FAILED);
		exit(1);
	}
	printf("Paging Success : SecMem %lu-->>Block %u\n", ptr_pageTabIt->auxAddr, blockNum);
}

/* 将被替换页面的内容写回辅存 */
void do_page_out(Ptr_PageTableItem ptr_pageTabIt)
{
	unsigned int writeNum;
	if (fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET) < 0)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt, ftell(ptr_auxMem));
#endif
		do_error(ERROR_FILE_SEEK_FAILED);
		exit(1);
	}
	if ((writeNum = fwrite(actMem + ptr_pageTabIt->blockNum * PAGE_SIZE,
		sizeof(BYTE), PAGE_SIZE, ptr_auxMem)) < PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG: auxAddr=%u\tftell=%u\n", ptr_pageTabIt->auxAddr, ftell(ptr_auxMem));
		printf("DEBUG: writeNum=%u\n", writeNum);
		printf("DEGUB: feof=%d\tferror=%d\n", feof(ptr_auxMem), ferror(ptr_auxMem));
#endif
		do_error(ERROR_FILE_WRITE_FAILED);
		exit(1);
	}
	printf("Write back Success : Block %lu-->>SecMem %03X\n", ptr_pageTabIt->auxAddr, ptr_pageTabIt->blockNum);
}
/* 错误处理 */
void do_error(ERROR_CODE code)
{
    //printf("%s\n",strerror(errno));
	switch (code)
	{
		case ERROR_READ_DENY:
		{
			printf("Access Failed : The address cannot be Read\n");
			break;
		}
		case ERROR_WRITE_DENY:
		{
			printf("Access Failed : The address cannot be Written\n");
			break;
		}
		case ERROR_EXECUTE_DENY:
		{
			printf("Access Failed : The address cannot be Executed\n");
			break;
		}
		case ERROR_INVALID_REQUEST:
		{
			printf("Access Failed : Illegal Request\n");
			break;
		}
		case ERROR_OVER_BOUNDARY:
		{
			printf("Access Failed : Address Cross-Border\n");
			break;
		}
		case ERROR_FILE_OPEN_FAILED:
		{
			printf("System Error:Open file failed\n");
			break;
		}
		case ERROR_FILE_CLOSE_FAILED:
		{
			printf("System Error:Close file failed\n");
			break;
		}
		case ERROR_FILE_SEEK_FAILED:
		{
			printf("System Error:File pointer position failed\n");
			break;
		}
		case ERROR_FILE_READ_FAILED:
		{
			printf("System Error:Read file failed\n");
			break;
		}
		case ERROR_FILE_WRITE_FAILED:
		{
			printf("System Error:Write file failed\n");
			break;
		}
		default:
		{
			printf("Unknown Error\n");
		}
	}
}



/* 打印页表 */
void do_print_info(int pro)
{
	unsigned int i;
	Ptr_ProPage p;
	char str[4];
	Ptr_PageTableItem pageTable;
	p=get_ptr_propage(pro);
	if(p==NULL)
	{
        printf("No page for process[%d] found.\n",pro);
	    return;
	}
	printf("\tPage Table for Process[%d]\n",pro);
	printf("P_No.\tB_No.\tLoad\tMod\tPro\tCnt\tSecMem\n");
	pageTable = p->pageTable;
	for (i = 0; i < PAGE_SUM; i++)
	{
		printf("%u\t%u\t%u\t%u\t%s\t%lu\t%lu\n", i, pageTable[i].blockNum, pageTable[i].filled,
			pageTable[i].edited, get_proType_str(str, pageTable[i].proType),
			pageTable[i].count, pageTable[i].auxAddr);
	}
}

/* 获取页面保护类型字符串 */
char *get_proType_str(char *str, BYTE type)
{
	if (type & READABLE)
		str[0] = 'r';
	else
		str[0] = '-';
	if (type & WRITABLE)
		str[1] = 'w';
	else
		str[1] = '-';
	if (type & EXECUTABLE)
		str[2] = 'x';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}

Ptr_ProPage get_ptr_propage(int pro)
{
    Ptr_ProPage p;
    if(head == NULL)
        return NULL;
    for(p=head;p!=NULL;p = p->next)
    {
        if(p->pro==pro)
        {
            //printf("Page for process[%d]\n",p->pro);
            break;
        }
    }
    return p;
}

/* 打印页表 */
void do_print_TLB()
{
	unsigned int i;
	char str[10];
	printf("\tTranslation Lookaside Buffer\n");
	printf("TLB_No.\tProc\tP_No.\tB_No.\tFeature\tPro\tCnt\n");
	for (i = 0; i < TLB_SUM; i++)
	{
		printf("%d\t%d\t%u\t%u\t%d\t%s\t%lu\n", i, TLB[i].pro, TLB[i].pageNum,
			TLB[i].blockNum, TLB[i].feature,get_proType_str(str, TLB[i].proType),
			TLB[i].count);
	}
}
void do_print_process()
{
    Ptr_ProPage p;
    p=head;
    if(!p)
        printf("No page for any process at present.\n");
    else
    {
        printf("Process chain: [%d]",p->pro);
        p=p->next;
        for(;p;p=p->next)
        {
            printf("-->[%d]",p->pro);
        }
        printf("\n");
    }
}
int main(int argc, char* argv[])
{
	char c[100];
	int count;
	for(count = 0;count<PAGE_SUM;count++)
    {
        useflag[count]=0;
        pagecount[count] = 0;
    }
    for(count=0;count<BLOCK_SUM;count++)
        blockStatus[count]=FALSE;

    ptr_propage = NULL;
    head = NULL;
	if (!(ptr_auxMem = fopen(AUXILIARY_MEMORY, "r+")))
	{
		do_error(ERROR_FILE_OPEN_FAILED);
		exit(1);
	}

	do_init();
	//do_print_info();

	//initial
	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
	/* 在循环中模拟访存请求与处理过程 */
	printf("Waiting for requests...\n");
	int timecount=0;
	while (TRUE)
	{
        timecount = (timecount+1)%16;
	    if(timecount==0)
	    {
            Ptr_ProPage p;
            for(p=head;p!=NULL;p=p->next)
            {
                int k;
                for(k=0;k<PAGE_SUM;k++)
                {
                    p->pageTable[k].count=(p->pageTable[k].count>>1)|( p->pageTable[k].useflag<<30);
                    p->pageTable[k].useflag=0;
                }
            }
	    }
	    if((count=read(fifo,ptr_memAccReq,sizeof(MemoryAccessRequest)))<0)
            printf("read fifo failed");
        else if(count>0)
        {
            do_response(ptr_memAccReq->pro);
            int ex=0;
            printf("%s",PROMPT);
            do{
                fgets(c,100,stdin);
                if(c[0]=='p'||c[0]=='P')
                {
                    if(c[1]=='c'||c[1]=='C')
                    {
                        do_print_process();
                        printf("%s",PROMPT);
                    }
                    else if(c[1]=='t'||c[1]=='T')
                    {
                        do_print_TLB();
                        printf("%s",PROMPT);
                    }
                    else if(c[1]==' ')
                    {
                        int pid=0;
                        int m;
                        if(!(c[2]<='9'&&c[2]>='0'))
                        {
                            printf("Prompt Error.\n");
                            continue;
                        }
                        for(m=2;c[m]<='9'&&c[m]>='0'&&m<strlen(c);m++)
                        {
                            pid*=10;
                            pid+=c[m]-'0';
                        }
                        do_print_info(pid);
                        printf("%s",PROMPT);
                    }
                    else
                    {
                        printf("Prompt Error.\n");
                    }
                }
                else if(c[0]=='e'||c[0]=='E')
                {
                    ex=1;
                    break;
                }
                else if(c[0]=='c'||c[0]=='C')
                {
                    break;
                }
                else if(c[0]=='\n')
                {
                    printf(">>");
                }
                else
                {
                    printf("Prompt Error.\n>>");
                }
            }while(1);
            if(ex==1)
                break;
        }
        else
		{
            continue;
		}
		//sleep(5000);
	}
	close(fifo);

	if (fclose(ptr_auxMem) == EOF)
	{
		do_error(ERROR_FILE_CLOSE_FAILED);
		exit(1);
	}

	return (0);
}
