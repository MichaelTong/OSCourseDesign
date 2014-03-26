#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif

    #define HISTORY_LEN 100

    #define STOPPED "STOPPED"
    #define RUNNING "RUNNING"
    #define DONE    "DONE"
    #define KILLED  "KILLED"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <math.h>
    #include <errno.h>
    #include <signal.h>
    #include <stddef.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/wait.h>
    #include <sys/ioctl.h>
    #include <sys/termios.h>

    typedef struct SimpleCmd {
        int isBack;     // 是否后台运行
        char **args;    // 命令及参数
        char *input;    // 输入重定向
        char *output;   // 输出重定向
    } SimpleCmd;
    typedef struct CompondCmd{
        int isBack;
        SimpleCmd **simCmd;
    } CompondCmd;
    typedef struct History {
        int start;                    //首位置
        int end;                      //末位置
        int cur;
        char cmds[HISTORY_LEN][100];  //历史命令
    } History;

    typedef struct Job {
        int pid;          //进程号
        char cmd[100];    //命令名
        char state[10];   //作业状态
        struct Job *next; //下一节点指针
        int isfg;
    } Job;

    typedef struct Recommend{
        unsigned char d_type;
        char *d_name;
    } Recommend;

    Recommend recommend[100];
    int recmdTop;
    char inputBuff[1000];  //存放输入的命令
    int nredirect;
    int bflag;
    int goon, ingnore;       //用于设置signal信号量
    char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
    History history;                 //历史命令
    Job *head;                //作业头指针
    pid_t fgPid;                     //当前前台作业的进程号
    Job *fHead,*fEnd;
    Job *fgHead;
    int state;
    void init();
    void addHistory(char *history);
    extern void execute(int isSimple);
    extern int handleInBuff(void);
    extern int tabFile(void);

    extern Job* addJob(pid_t pid);
    extern void rmJob(int sig, siginfo_t *sip, void* noused);
    extern void ctrl_C();
    extern void ctrl_Z();

    int tabflag;int isSimple;
#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */
