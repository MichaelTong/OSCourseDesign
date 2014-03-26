#include "global.h"

/*******************************************************
                     初始化环境
********************************************************/
/*通过路径文件获取环境路径*/
extern void getEnvPath(int len, char *buf){
    int i, j, pathIndex = 0, temp;
    char path[40];

    for(i = 0, j = 0; i < len; i++){
        if(buf[i] == ':'){ //将以冒号(:)分隔的查找路径分别设置到envPath[]中
            if(path[j-1] != '/'){
                path[j++] = '/';
            }
            path[j] = '\0';
            j = 0;

            temp = strlen(path);
            envPath[pathIndex] = (char*)malloc(sizeof(char) * (temp + 1));
            strcpy(envPath[pathIndex], path);

            pathIndex++;
        }else{
            path[j++] = buf[i];
        }
    }

    envPath[pathIndex] = NULL;
}

/*初始化操作*/
extern void init(){
    int fd, len;
    char c, buf[80];

	//打开查找路径文件ysh.conf
    if((fd = open("ysh.conf", O_RDONLY, 660)) == -1){
        perror("init environment failed\n");
        exit(1);
    }

	//初始化history链表
    history.end = -1;
    history.start = 0;
    history.cur = -1;
    bflag=1;
    goon=0;
    ingnore=0;
    head=NULL;
    fHead=NULL;
    tabflag=0;
    nredirect=0;
    isSimple=1;

    len = 0;
	//将路径文件内容依次读入到buf[]中
    while(read(fd, &c, 1) != 0){
        buf[len++] = c;
    }
    buf[len] = '\0';

    //将环境路径存入envPath[]
    getEnvPath(len, buf);

    //注册信号
    struct sigaction action;
    action.sa_sigaction = rmJob;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);
    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT, ctrl_C);
}
