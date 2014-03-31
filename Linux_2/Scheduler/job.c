#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job.h"
//#define DEBUG
int jobid=0;
int siginfo=1;
int fifo,statfifo;
int globalfd;
int debugcount=0;
/*priruntime ��ʾ��ǰ�����ѻ���ʱ�䣬currentpri��ʾ��ǰ���ڶ��У�����ǰ���ȼ�*/
int priruntime=0,currentpri=0;
int queuetime[4] = {16,8,4,2};

struct waitqueue *head=NULL;
struct waitqueue *next=NULL,*current =NULL;

/* ���ȳ��� */
void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	int  count = 0;
	int i=0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
#ifdef DEBUG

	if(count){
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	}
	else
		printf("no data read\n");
#endif

	/* ���µȴ������е���ҵ */
#ifdef DEBUG
	if(debugcount==2)
		sleep(1);
#endif
	updateall();

	switch(cmd.type){
	case ENQ:
#ifdef DEBUG
		debugcount++;
#endif
		do_enq(newjob,cmd);
		break;
	case DEQ:
		do_deq(cmd);
		break;
	case STAT:
		do_stat(cmd);
		break;
	default:
		break;
	}

	/* ѡ������ȼ���ҵ */
	next=jobselect();
	/* ��ҵ�л� */
	jobswitch();
}

int allocjid()
{
	return ++jobid;
}

void updateall()
{
	struct waitqueue *p;

	/* ������ҵ����ʱ�� */
	if(current)
    {
        priruntime++;
        current->job->run_time += 1; /* ��1����1000ms */
        if(priruntime >= queuetime[currentpri])
        {
            if(currentpri == 0)
                current->job->curpri = current->job->defpri;
            else
                current->job->curpri--;
        }
        //current->job->curpri--; //��ǰ�������ȼ���1
    }
	/* ������ҵ�ȴ�ʱ�估���ȼ� */
	for(p = head; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(current && priruntime >= queuetime[currentpri] && p->job->curpri == currentpri)
        {
            p->job->curpri--;
        }
		/*if(p->job->wait_time >= 5000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
		}*/
	}
	if(current)
	{
        if(priruntime >= queuetime[currentpri])
        {
            priruntime = 0;
            currentpri--;
        }
	}
}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	selectprev = NULL;
	if(head){
		if(currentpri == -1)
		{
		    for(p=head;p!=NULL;p=p->next)
		    {
		        p->job->curpri=p->job->defpri;
		    }
		    //currentpri = 0;
		}
		/* �����ȴ������е���ҵ���ҵ����ȼ���ߵ���ҵ */
		for(prev = head, p = head; p != NULL; prev = p,p = p->next)
		{
		    if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}
			//printf("%d\t%d\t%d\n",p->job->pid,p->job->curpri,p->job->defpri);
		}
		if(select->job->curpri < currentpri)
            return NULL;
        currentpri = select->job->curpri;
		if(selectprev != select)
            selectprev->next = select->next;
        if (select == selectprev)
            head = head->next;
	}
	return select;
}

void jobswitch()
{
	struct waitqueue *p;
	int i;

	if(current && current->job->state == DONE){ /* ��ǰ��ҵ��� */
		/* ��ҵ��ɣ�ɾ���� */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* �ͷſռ� */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) /* û����ҵҪ���� */
        return;
	else if (next != NULL && current == NULL){ /* ��ʼ�µ���ҵ */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL){ /* �л���ҵ */

		printf("switch to Pid: %d %d %d\n",next->job->pid,currentpri,next->job->curpri);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;

		current->next = NULL;

		/* �Żصȴ����� */
		if(head != NULL){
			for(p = head; p->next != NULL; p = p->next);
			p->next = current;
		}else{
			head = current;
		}
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}else{ /* next == NULL��current != NULL�����л� */
		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;

	switch (sig) {
case SIGVTALRM: /* �����ʱ�������õļ�ʱ��� */
	scheduler();
	return;
case SIGCHLD: /* �ӽ��̽���ʱ���͸������̵��ź� */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0)
		return;
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
	}else if (WIFSTOPPED(status)){
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));
	}
	return;
	default:
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

	sigemptyset(&zeromask);

	/* ��װjobinfo���ݽṹ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}else
			offset++;
	}

	arglist[i] = NULL;

#ifdef DEBUG

	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);

#endif

	/*��ȴ������������µ���ҵ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;

	if(head)
	{
		for(p=head;p->next != NULL; p=p->next);
		p->next =newnode;
	}else
		head=newnode;

	/*Ϊ��ҵ��������*/
	if((pid=fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid =getpid();
		/*�����ӽ���,�ȵ�ִ��*/
		raise(SIGSTOP);
#ifdef DEBUG

		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		/*�����ļ�����������׼���*/
		dup2(globalfd,1);
		/* ִ������ */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		newjob->pid=pid;
	}
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);

#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif

	/*current jodid==deqid,��ֹ��ǰ��ҵ*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* �����ڵȴ������в���deqid */
		select=NULL;
		selectprev=NULL;
		if(head){
			for(prev=head,p=head;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
			if(select==NULL)//���벻������Ϣ
				return;
			selectprev->next=select->next;
			if(select==selectprev && select->next == NULL)
				head=NULL;
			else if(select==selectprev)
				head=select->next;
		}
		if(select){
			for(i=0;(select->job->cmdarg)[i]!=NULL;i++){
				free((select->job->cmdarg)[i]);
				(select->job->cmdarg)[i]=NULL;
			}
			free(select->job->cmdarg);
			free(select->job);
			free(select);
			select=NULL;
		}
	}
}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	char statstr[BUFLEN*3];
	struct stat statbuf;
	char fifoname[BUFLEN];
	int fd;
	/*����FIFO������Ϣ*/
	sprintf(fifoname,"/tmp/stat-tmp-uid-%d",statcmd.owner);
	if(stat(fifoname,&statbuf)==0){
		/* ���FIFO�ļ�����,ɾ�� */
		if(remove(fifoname)<0)
			error_sys("remove failed");
	}

	if(mkfifo(fifoname,0666)<0)
		error_sys("mkfifo failed");
	/* �ڷ�����ģʽ�´�FIFO */
	
	
	/*
	*��ӡ������ҵ��ͳ����Ϣ:
	*1.��ҵID
	*2.����ID
	*3.��ҵ������
	*4.��ҵ����ʱ��
	*5.��ҵ�ȴ�ʱ��
	*6.��ҵ����ʱ��
	*7.��ҵ״̬
	*/

	/* ��ӡ��Ϣͷ�� */
	
	if((fd=open(fifoname,O_WRONLY))<0)
		error_sys("stat open fifo failed");
		
	sprintf(statstr,"JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	
	if(write(fd,statstr,BUFLEN*3)<0)
		error_sys("stat write failed");
		
	if(current){
		memset(timebuf,0,BUFLEN);
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
	
		sprintf(statstr,"%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
		
		if(write(fd,statstr,BUFLEN*3)<0)
			error_sys("stat write failed");
	}

	for(p=head;p!=NULL;p=p->next){
		memset(timebuf,0,BUFLEN);
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		sprintf(statstr,"%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
		if(write(fd,statstr,BUFLEN*3)<0)
			error_sys("stat write failed");
	}
	if(write(fd,"finished",BUFLEN*3)<0)
		error_sys("stat write failed");
	close(fd);
}

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;

	if(stat("/tmp/server",&statbuf)==0){
		/* ���FIFO�ļ�����,ɾ�� */
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}

	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	/* �ڷ�����ģʽ�´�FIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	/* �����źŴ������� */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* ����ʱ����Ϊ1000���� */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(globalfd);
	return 0;
}