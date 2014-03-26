#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"

/* 
 * 命令语法格式
 *     stat
 */
void usage()
{
	printf("Usage: stat\n");		
}

int main(int argc,char *argv[])
{
	struct jobcmd statcmd;
	int fd;
	int count;
	int statfifo;
	struct stat statbuf;
	char statstr[BUFLEN*3];
	char fifoname[BUFLEN];
	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;
	//命令写入fifo
	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("stat open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat write failed");
	sprintf(fifoname,"/tmp/stat-tmp-uid-%d",getuid());
	while(1){
		if(stat(fifoname,&statbuf)==0)
			break;
	}
	//从statfifo读入
	if((statfifo=open(fifoname,O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");
	
	while(!(read(statfifo,statstr,BUFLEN*3)>0)){
		//printf("%s",statstr);
	}
	printf("%s",statstr);
	while(1){
		if(read(statfifo,statstr,BUFLEN*3)>0)
		{
			if(strcmp(statstr,"finished")==0)
				break;
			printf("%s",statstr);
		}
	}
	close(statfifo);
	if(stat(fifoname,&statbuf)==0){
		/* 如果FIFO文件存在,删掉 */
		if(remove(fifoname)<0)
			error_sys("remove failed");
	}
	close(fd);
	return 0;
}
