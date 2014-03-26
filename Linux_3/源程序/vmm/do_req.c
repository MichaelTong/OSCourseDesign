#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>
#include <error.h>
#include <errno.h>

#include "do_req.h"

Ptr_MemoryAccessRequest ptr_memAccReq;

/* 产生访存请求 */
void do_request()
{
	char buf[1000],buf2[1000];
	int pageNum, offAddr;
	/* 设置pid位，标识进程 */
	ptr_memAccReq->pid = getpid();
	sprintf(buf,"<<Process-%d>>GenReq-- ",ptr_memAccReq->pid);
	/* 随机产生请求地址 */
	ptr_memAccReq->virAddr = random() % VIRTUAL_MEMORY_SIZE;
	pageNum = ptr_memAccReq->virAddr / PAGE_SIZE;
	offAddr = ptr_memAccReq->virAddr % PAGE_SIZE;
	/* 随机产生请求类型 */
	switch (random() % 3)
	{
		case 0: //读请求
		{
			ptr_memAccReq->reqType = REQUEST_READ;
			sprintf(buf2,"Addr : %lu[%d:%d %X:%X]\tType : Read\n", ptr_memAccReq->virAddr,pageNum, offAddr,pageNum, offAddr);
			break;
		}
		case 1: //写请求
		{
			ptr_memAccReq->reqType = REQUEST_WRITE;
			/* 随机产生待写入的值 */
			ptr_memAccReq->value = random() % 0xFFu;
			sprintf(buf2,"Addr : %lu[%d:%d %X:%X]\tType : Write\tValue : %02X\n", ptr_memAccReq->virAddr,pageNum, offAddr,pageNum, offAddr, ptr_memAccReq->value);
			break;
		}
		case 2:
		{
			ptr_memAccReq->reqType = REQUEST_EXECUTE;
			sprintf(buf2,"Addr : %lu[%d:%d %X:%X]\tType : Execute\n", ptr_memAccReq->virAddr,pageNum, offAddr,pageNum, offAddr);
			break;
		}
		default:
			break;
	}
	strcat(buf,buf2);
	printf("%s",buf);
}
void do_over()
{
	char buf[1000];
	ptr_memAccReq->pid = getpid();
	sprintf(buf,"<<Process-%d>>GenReq-- ",ptr_memAccReq->pid);
	ptr_memAccReq->reqType = REQUEST_OVER;
	strcat(buf,"Over\n");
	printf("%s",buf);
}
void usage()
{
	printf("Usage: do_req (-p p_num [-r r_num] | -m)\n\tdo_req\t: App to generate requests.\n\t-p\t: Process(es).\n\tp_num\t: Digit only, number of processes (>0).\n\t-r\t: Optional, request(s).\n\tr_num\t: Digit only, number of requests (>0), default by 10.\n\t-m\t: Manual mode, one process\n");
	exit(0);
}

int digit(char* argv)
{
	int i=0;
	int len=strlen(argv);
	int count=0;
	for(;i<len;i++)
	{
		if(!(argv[i]>='0'&&argv[i]<='9'))
		{
			printf("Error: Invalid argument.\n");
			usage();
		}
		count*=10;
		count+=argv[i]-'0';
	}
	return count;
}

void createChild(int r_cnt)
{
	int fd;
	pid_t pid;
	pid=fork();
	if(pid==0)
	{
		srandom(getpid());
		while(r_cnt>0)
		{
			do_request();
			if((fd=open("/tmp/vmm.temp",O_WRONLY))<0)
				printf("req open fifo failed\n");
			if(write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)
				printf("Req write failed\n");
			close(fd);
			r_cnt--;
			int t;
			t=random()%2000;
			usleep(t);
		}
		do_over();
		if((fd=open("/tmp/vmm.temp",O_WRONLY))<0)
			printf("req open fifo failed\n");
		if(write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)
			printf("Req write failed\n");
		close(fd);
		exit(0);
	}
	return;
}

int main(int argc, char* argv[])
{
	int i,at,r_cnt=0,p_cnt=0;
	if(argc==1||argc==4){
		printf("Error: Too few arguments.\n");
		usage();
	}
	else if(argc==2){
		if(*argv[1]!='-')
		{
			printf("Error: Invalid argument.\n");
			usage();
		}
		if(argv[1][1]!='m')
		{
			printf("Error: Invalid argument. \nOption: -m\n");
			usage();
		}
		at=0;
	}
	else if(argc==3||argc==5){
		if(*argv[1]!='-')
		{
			printf("Error: Invalid argument.\n");
			usage();
		}
		if(argv[1][1]!='p')
		{
			printf("Error: Invalid argument. \nOption: -p\n");
			usage();
		}
		p_cnt=digit(argv[2]);
		r_cnt=10;
		at=1;
		if(argc==5)
		{
			if(*argv[3]!='-')
			{
				printf("Error: Invalid argument.\n");
				usage();
			}
			if(argv[3][1]!='r')
			{
				printf("Error: Invalid argument. \nOption: -r\n");
				usage();
			}
			r_cnt=digit(argv[4]);
		}
		if(p_cnt==0)
		{
			printf("Error: Invalid argument. \np_cnt > 0\n");
			usage();
		}
		if(r_cnt==0)
		{
			printf("Error: Invalid argument. \nr_cnt > 0\n");
			usage();
		}
	}
	else if(argc>5){
		printf("Error: Too many arguments.\n");
		usage();
	}

	ptr_memAccReq = (Ptr_MemoryAccessRequest) malloc(sizeof(MemoryAccessRequest));
    	srandom(time(NULL));
	if(at)
	{
		for(i=0;i<p_cnt;i++)
		{
			createChild(r_cnt);
		}
		pid_t childPid;
		int waitStatus;
		while( (childPid = wait(&waitStatus) ) > 0 )
    		{

    		}
	}
	else
	{
		while(1){
			char c;
			int fd;
			do_request();
			if((fd=open("/tmp/vmm.temp",O_WRONLY))<0)
				printf("req open fifo failed\n");
			if(write(fd,ptr_memAccReq,sizeof(MemoryAccessRequest))<0)
				printf("Req write failed\n");
			close(fd);
			printf("Press 'X' to exit, or other keys to continue...\n");
            		if ((c = getchar()) == 'x' || c == 'X')
                		break;
            		while (c != '\n')
                		c = getchar();
		}
	}

}


