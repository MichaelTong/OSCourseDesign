#include "global.h"

/*******************************************************
                        jobs相关
********************************************************/
/*添加新的作业*/
extern Job* addJob(pid_t pid){
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));

	//初始化新的job
    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;
    job->isfg=0;
    if(head == NULL){ //若是第一个job，则设置为头指针
        head = job;
    }else{ //否则，根据pid将新的job插入到链表的合适位置
		now = head;
		while(now != NULL && now->pid < pid){
			last = now;
			now = now->next;
		}
        last->next = job;
        job->next = now;
    }

    return job;
}

extern Job* addFgJob(pid_t pid,int isfg){
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));

	//初始化新的job
    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;
    job->isfg=isfg;
    if(fgHead == NULL){ //若是第一个job，则设置为头指针
        fgHead = job;
    }else{ //否则，根据pid将新的job插入到链表的合适位置
		now = fgHead;
		while(now != NULL && now->pid < pid){
			last = now;
			now = now->next;
		}
        last->next = job;
        job->next = now;
    }

    return job;
}
extern Job* findFgJob(pid_t pid){//查找前台作业

    Job *now;
    now=fgHead;
    while(now!=NULL){
        if(now->pid==pid)
            return now;
        now=now->next;
    }
    return now;

}
/*移除一个作业*/
extern void rmJob(int sig, siginfo_t *sip, void* noused){
    pid_t pid;
    Job *now = NULL, *last = NULL;
    char temp[10];
    if(ingnore == 1){
        ingnore = 0;
        return;
    }

    pid = sip->si_pid;

    now = head;
	while(now != NULL && now->pid < pid){
		last = now;
		now = now->next;
	}

    if(now == NULL){ //作业不存在，则不进行处理直接返回
        now = fgHead;
		while(now != NULL && now->pid < pid){
			last = now;
			now = now->next;
		}
		if(now!=NULL){
            if(now==fgHead)
                fgHead=now->next;
            else
                last->next=now->next;
		}
		free(now);
        return;
    }

	//开始移除该作业,移动到完成作业表
    if(now == head){
        head = now->next;
    }
    else{
        last->next = now->next;
    }

    if(fHead==NULL){
        fEnd = fHead = now;
    }
    else{
        fEnd->next = now;
    }
    sprintf(temp,"%d",state);
    strcpy(now->state, DONE);
    strcat(now->state, " ");
    strcat(now->state, temp);
    bflag = 2;
}
