#include "global.h"
#include <dirent.h>
extern void fg_exec(int pid);
extern void bg_exec(int pid);
extern void getRegex(int* b, int* e,char* path);
extern void regexChange(char *origin,char *newarg,unsigned char d_type);
extern int regexNewArgs(char *origin, char **args,int *index);
extern int regexNum(char *origin);
/*******************************************************
                  工具以及辅助方法
********************************************************/
/*判断命令是否存在*/
int exists(char *cmdFile){
    int i = 0;
    if((cmdFile[0] == '/' || cmdFile[0] == '.') && access(cmdFile, F_OK) == 0){ //命令在当前目录
        strcpy(cmdBuff, cmdFile);
        return 1;
    }else{  //查找ysh.conf文件中指定的目录，确定命令是否存在
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmdFile);

            if(access(cmdBuff, F_OK) == 0){ //命令文件被找到
                return 1;
            }

            i++;
        }
    }

    return 0;
}
int getCompond(char *cmdFile,char *cmdB){
    int i = 0;
    if((cmdFile[0] == '/' || cmdFile[0] == '.') && access(cmdFile, F_OK) == 0){ //命令在当前目录
        strcpy(cmdB, cmdFile);
        return 1;
    }else{  //查找ysh.conf文件中指定的目录，确定命令是否存在
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            strcpy(cmdB, envPath[i]);
            strcat(cmdB, cmdFile);

            if(access(cmdB, F_OK) == 0){ //命令文件被找到
                return 1;
            }

            i++;
        }
    }

    return 0;
}


/*将字符串转换为整型的Pid*/
int str2Pid(char *str, int start, int end){
    int i, j;
    char chs[20];

    for(i = start, j= 0; i < end; i++, j++){
        if(str[i] < '0' || str[i] > '9'){
            return -1;
        }else{
            chs[j] = str[i];
        }
    }
    chs[j] = '\0';

    return atoi(chs);
}

/*调整部分外部命令的格式*/
void justArgs(char *str){
    int i, j, len;
    len = strlen(str);

    for(i = 0, j = -1; i < len; i++){
        if(str[i] == '/'){
            j = i;
        }
    }

    if(j != -1){ //找到符号'/'
        for(i = 0, j++; j < len; i++, j++){
            str[i] = str[j];
        }
        str[i] = '\0';
    }
}

/*设置goon*/
void setGoon(){
    goon = 1;
}

/*释放环境变量空间*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}


/*******************************************************
                    命令历史记录
********************************************************/
void addHistory(char *cmd){
    if(history.end == -1){ //第一次使用history命令
        history.end = 0;
        strcpy(history.cmds[history.end], cmd);
        history.cur=history.end;
        return;
	}

    history.end = (history.end + 1)%HISTORY_LEN; //end前移一位
    strcpy(history.cmds[history.end], cmd); //将命令拷贝到end指向的数组中

    if(history.end == history.start){ //end和start指向同一位置
        history.start = (history.start + 1)%HISTORY_LEN; //start前移一位
    }
    history.cur=history.end;
}



/*******************************************************
                      命令解析
********************************************************/
void freeSimpleCmd(SimpleCmd *scmd){
    int i;
    free(scmd->input);
    free(scmd->output);
    scmd->input=NULL;
    scmd->output=NULL;
    for(i = 0; scmd->args[i] != NULL; i++){
        free(scmd->args[i]);
        scmd->args[i]=NULL;
    }
    free(scmd->args);
    scmd->args=NULL;
}

void freeCompondCmd(CompondCmd *ccmd){
    int i;
    for(i=0;ccmd->simCmd[i]!=NULL;i++){
        freeSimpleCmd(ccmd->simCmd[i]);
        ccmd->simCmd[i]=NULL;
    }
    free(ccmd->simCmd);
    ccmd->simCmd=NULL;
}


SimpleCmd* handleSimpleCmdStr(int begin, int end){
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));

	//默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;

    //初始化相应变量
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';

    i = begin;
	//跳过空格等无用信息
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
        i++;
    }

    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end){
		/*根据命令字符的不同情况进行不同的处理*/
        switch(inputBuff[i]){
            case ' ':
            case '\t': //命令名及参数的结束标志
                temp[j] = '\0';
                j = 0;
                if(!fileFinished){
                    k++;
                    temp = buff[k];
                }
                break;

            case '<': //输入重定向标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = inputFile;
                fileFinished = 1;
                i++;
                break;

            case '>': //输出重定向标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = outputFile;
                fileFinished = 1;
                i++;
                break;

            case '&': //后台运行标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                cmd->isBack = 1;
                fileFinished = 1;
                i++;
                break;

            default: //默认则读入到temp指定的空间
                temp[j++] = inputBuff[i++];
                continue;
		}

		//跳过空格等无用信息
        while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
            i++;
        }
	}

    if(inputBuff[end-1] != ' ' && inputBuff[end-1] != '\t' && inputBuff[end-1] != '&'){
        temp[j] = '\0';
        if(!fileFinished){
            k++;
        }
    }

	//依次为命令名及其各个参数赋值
	if(strstr(buff[i],"*")!=NULL||strstr(buff[i],"?")){
        printf("Can't execute %s\n",buff[i]);
        return NULL;
	}
    cmd->args = (char**)malloc(sizeof(char*) * (k + 1));
    cmd->args[k] = NULL;
    for(i = 0; i<k; i++){
        j = strlen(buff[i]);
        cmd->args[i] = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->args[i], buff[i]);
    }

	//如果有输入重定向文件，则为命令的输入重定向变量赋值
    if(strlen(inputFile) != 0){
        j = strlen(inputFile);
        cmd->input = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->input, inputFile);
    }

    //如果有输出重定向文件，则为命令的输出重定向变量赋值
    if(strlen(outputFile) != 0){
        j = strlen(outputFile);
        cmd->output = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->output, outputFile);
    }

    return cmd;
}

CompondCmd* handleCompondCmdStr(int begin, int end){
    int i,j,k;
    CompondCmd *cmd;
    cmd=(CompondCmd*)malloc(sizeof(CompondCmd));
    cmd->simCmd=(SimpleCmd**)malloc((nredirect+2)*sizeof(SimpleCmd*));
    cmd->simCmd[nredirect+1]=NULL;
    cmd->isBack=0;
    j=i=begin;
    k=0;
    while(j<end){
        if(inputBuff[j]!='|'){
            j++;
            continue;
        }
        cmd->simCmd[k]=handleSimpleCmdStr(i,j);
        k++;
        i=j+1;
        j++;
    }
    cmd->simCmd[k]=handleSimpleCmdStr(i,j);
    return cmd;
}



/*******************************************************
                      命令执行
********************************************************/
int prepareOutCmd(SimpleCmd *cmd){
    int pipeIn, pipeOut;
    if(cmd->input != NULL){ //存在输入重定向
        if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
            printf("Could not open %s!\n", cmd->input);
            return -1;
        }
        if(dup2(pipeIn, 0) == -1){
            printf("Input error!\n");
            return -1;
        }
        close(pipeIn);
    }

    if(cmd->output != NULL){ //存在输出重定向
        if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
            printf("could not open %s！\n", cmd->output);
            return -1;
        }
        if(dup2(pipeOut, 1) == -1){
            printf("output error！\n");
            return -1;
        }
        close(pipeOut);
    }
    justArgs(cmd->args[0]);
    return 0;
}
/*执行外部命令*/
int execOuterCmd(SimpleCmd *cmd){
    pid_t pid;

    if(exists(cmd->args[0])){ //命令存在
        int i=1;
        int num=1;
        while(cmd->args[i]!=NULL){
            if(strstr(cmd->args[i],"*")!=NULL||strstr(cmd->args[i],"?"))
                num=num+regexNum(cmd->args[i]);
            else
                num++;
            i++;
        }
        char **newargs=(char**)malloc((num+1)*sizeof(char*));
        i=0;
        newargs[num]=NULL;
        int index=-1;
        while(cmd->args[i]!=NULL){
            if(strstr(cmd->args[i],"*")!=NULL||strstr(cmd->args[i],"?"))
                regexNewArgs(cmd->args[i],newargs,&index);
            else{
                newargs[++index]=(char*)malloc((strlen(cmd->args[i])+1)*sizeof(char));
                strcpy(newargs[index],cmd->args[i]);
            }
            i++;
        }
        i=0;
        while(cmd->args[i]!=NULL){
            free(cmd->args[i]);
            i++;
        }
        free(cmd->args);
        cmd->args=newargs;

        signal(SIGUSR1, setGoon);

        if((pid = fork()) < 0){
            perror("fork failed");
            return -1;
        }

        if(pid == 0){ //子进程
            if(prepareOutCmd(cmd)==-1){
                return -1;
            }
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                //while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                //goon = 0; //置0，为下一命令做准备
                while(!goon)
                    sleep(1);
                goon=0;
                kill(getppid(), SIGUSR1);
            }
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return -1;
            }
        }
		else{ //父进程
            if(cmd ->isBack){ //后台命令
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                waitpid(pid, &state, WNOHANG);
                //等待子进程输出
                while(!goon)
                    sleep(1);
                goon = 0;
                printf("[%d]\t%s\t\t%s\n", pid, RUNNING, inputBuff);
            }
            else{ //非后台命令
                fgPid = pid;
                addFgJob(pid,0);
                waitpid(pid, NULL, 0);
            }
		}
    }
    else{ //命令不存在
        printf("Could not find command : %s\n", inputBuff);
        return -1;
    }
    return 0;
}

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd){
    int i, pid;
    char *temp;
    Job *now = NULL;
    if(cmd==NULL)
        return ;
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0) { //history命令
        if(history.end == -1){
            printf("No command has been executed.\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        if(head == NULL&&fHead==NULL){
            printf("There is no jobs.\n");
        } else {
            printf("INDEX\tPID\tSTATE\t\tCOMMAND\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
            for(now = fHead; now != NULL; i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
                now=now->next;
                free(fHead);
                fHead=now;
            }
            bflag=1;
        }
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        temp = cmd->args[1];
        char temp2[100];
        regexChange(temp,temp2,DT_DIR);
        if(temp2 != NULL){
            if(chdir(temp2) < 0){
                printf("cd : couln't find folder \"%s\".\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg : illegal arguements, the correct format is:fg %%<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));

            if(pid != -1){
                bg_exec(pid);
            }
        }
		else{
            printf("bg : illegal arguements, the correct format is:bg %%<int>\n");
        }
    } else{ //外部命令
        execOuterCmd(cmd);
    }

    freeSimpleCmd(cmd);
}


int execCompondCmd(CompondCmd *cmd){
    int **fd=(int**)malloc((nredirect)*sizeof(int*));
    pid_t pgid=0,pid=0;
    int i;
    int cmdc=0;
    if(cmd==NULL){
        free(fd);
        return -1;
    }

    //Parent creat all pipes needed at beginning
    for( i=0;i<nredirect;i++){//分配管道
        fd[i]=(int*)malloc(2*sizeof(int));
        if(pipe(fd[i])<0){
            printf("pipe error.\n");
            return -1;
        }
    }
    for(i=0;i<=nredirect;i++){//判断命令是否存在
        if(!exists(cmd->simCmd[i]->args[0])){
        printf("Could not find command or execute internal command: %s\n", cmd->simCmd[i]->args[0]);
        return -1;
        }
    }
    while(cmdc<nredirect+1){
        pid=fork();
        if(pid==0)
            break;
        //pList[cmdc]=pid;
        if(cmdc==0)
            pgid=pid;
        //setpgid(pid,pgid);
        cmdc++;
    }
    if(pid==0){//子进程管道重定向
        if(cmdc!=0){
            if(dup2(fd[cmdc-1][0],0)<0){
                printf("redirect error.\n");
                return -1;
            }
        }
        if(cmdc!=nredirect){
            if(dup2(fd[cmdc][1],1)<0){
                printf("redirect error.\n");
                return -1;
            }
        }
        for(i=0;i<nredirect;i++){
            if(!(i!=0&&i==cmdc-1)){
                close(fd[i][0]);
            }
            if(!(i!=nredirect&&i==cmdc)){
                close(fd[i][1]);
            }
        }//add back ground
        /*if(cmdc==0){
            sigset_t ttou_set;
            sigset_t old_sig;
            sigaddset(&ttou_set,SIGTTOU);
            sigprocmask(SIG_SETMASK,&ttou_set,&old_sig);
            tcsetpgrp(STDIN_FILENO,getpid());
            sigprocmask(SIG_SETMASK,&old_sig,NULL);
        }*/
        if(prepareOutCmd(cmd->simCmd[cmdc])==-1){
                return -1;
        }
        char cmdB[100];
        memset(cmdB,0,100);
        getCompond(cmd->simCmd[cmdc]->args[0],cmdB);
        if(execv(cmdB, cmd->simCmd[cmdc]->args) < 0){ //执行命令
            printf("execv failed!\n");
            return -1;
        }
    }
    else{
        for(i=0;i<nredirect;i++){
            close(fd[i][0]);
            close(fd[i][1]);
        }
        fgPid=pgid;
        addFgJob(pgid,1);
        waitpid(pgid,NULL,0);
        //tcsetpgrp(STDIN_FILENO,getpid());
    }
    for(i=0;i<nredirect;i++)
        free(fd[i]);
    free(fd);
    freeCompondCmd(cmd);
    return 0;
}
/*
void waitfg(){
    int wPid,waitNum=0;
    int st;
    while(wPid = waitpid(fgPid, &st, WUNTRACED|WCONTINUED)){
        if(!WIFCONTINUED(status))
            waitNum++;
        if(waitNum==fgJob->cmd->cmdCount)
            break;
    }
}*/
/*******************************************************
                     命令执行接口
********************************************************/
void execute(int isSimple){
    if(isSimple){
        SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
        execSimpleCmd(cmd);
    }
    else{
        CompondCmd *cmd = handleCompondCmdStr(0,strlen(inputBuff));
        execCompondCmd(cmd);
    }
}
