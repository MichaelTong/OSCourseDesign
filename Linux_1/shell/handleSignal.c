#include "global.h"
#include <errno.h>
/*组合键命令ctrl+z*/
extern void ctrl_Z(){
    Job *now = NULL;

    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }

    //SIGCHLD信号产生自ctrl+z
    ingnore = 1;

	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;

    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }

	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED);
    strcat(now->cmd,"&");
    printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	//发送SIGSTOP信号给正在前台运作的工作，将其停止
	Job *temp=findFgJob(fgPid);
	if(temp->isfg)
        killpg(fgPid, SIGSTOP);
	else
        kill(fgPid, SIGSTOP);
    //printf("sssss\n");
    fgPid = 0;
}
/*组合键命令ctrl+c*/
extern void ctrl_C(){//

    if(fgPid == 0){
        printf("\nxsh@%s>", get_current_dir_name());//前台没有作业则直接返回
    }
    else{
        Job *temp=findFgJob(fgPid);
        if(temp->isfg)
            killpg(fgPid, SIGKILL);//进程组处理
        else
            kill(fgPid, SIGKILL);

        printf("\n");
        fgPid = 0;
    }
    return;
}

/*fg命令*/
extern void fg_exec(int pid){
    Job *now = NULL;
	int i;
    int status=0;
    //int temp;
    //SIGCHLD信号产生自此函数
    ingnore = 1;

	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;

    if(now == NULL){ //未找到作业
        printf("The job whose pid is %d dose not exist！\n", pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);

    signal(SIGTSTP, ctrl_Z); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    printf("%s\n", now->cmd);
    Job *temp=findFgJob(fgPid);
    if(temp->isfg)
        killpg(fgPid, SIGCONT);
    else
        kill(fgPid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    //while(1){
    temp=waitpid(fgPid,&status, 0); //父进程等待前台进程的运行
    //printf("\n%d\nreturn...",temp);
    //printf("\n%d\nstatus...",status);
    //printf("\n%d\nWIFEXITED...",WIFEXITED(status));
    //if(WIFEXITED(status))
    //    printf("\n%d\nWEXITSTATUS...",WEXITSTATUS(status));
    //printf("\n%d\nWIFSIGNALED...",WIFSIGNALED(status));
    //printf("\n%d\nWIFSTOPPED...",WIFSTOPPED(status));
    //printf("\n%d\nErrno...",errno);
    /*)    if(temp==-1){
            if(errno==EINTR)
                continue;
            else
                break;
        }
        else if(temp==0)
            break;
    }*/
    temp=waitpid(fgPid,&status, 0);
}


/*bg命令*/
extern void bg_exec(int pid){
    Job *now = NULL;

    //SIGCHLD信号产生自此函数
    ingnore = 1;

	//根据pid查找作业
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;

    if(now == NULL){ //未找到作业
        printf("The job whose pid is %d dose not exist！\n", pid);
        return;
    }

    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);\

    Job *temp=findFgJob(now->pid);
    if(temp->isfg)
        killpg(now->pid, SIGCONT);
    else
        kill(now->pid, SIGCONT);//向对象作业发送SIGCONT信号，使其运行
}
