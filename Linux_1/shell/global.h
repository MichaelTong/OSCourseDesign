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
        int isBack;     // �Ƿ��̨����
        char **args;    // �������
        char *input;    // �����ض���
        char *output;   // ����ض���
    } SimpleCmd;
    typedef struct CompondCmd{
        int isBack;
        SimpleCmd **simCmd;
    } CompondCmd;
    typedef struct History {
        int start;                    //��λ��
        int end;                      //ĩλ��
        int cur;
        char cmds[HISTORY_LEN][100];  //��ʷ����
    } History;

    typedef struct Job {
        int pid;          //���̺�
        char cmd[100];    //������
        char state[10];   //��ҵ״̬
        struct Job *next; //��һ�ڵ�ָ��
        int isfg;
    } Job;

    typedef struct Recommend{
        unsigned char d_type;
        char *d_name;
    } Recommend;

    Recommend recommend[100];
    int recmdTop;
    char inputBuff[1000];  //������������
    int nredirect;
    int bflag;
    int goon, ingnore;       //��������signal�ź���
    char *envPath[10], cmdBuff[40];  //�ⲿ����Ĵ��·������ȡ�ⲿ����Ļ���ռ�
    History history;                 //��ʷ����
    Job *head;                //��ҵͷָ��
    pid_t fgPid;                     //��ǰǰ̨��ҵ�Ľ��̺�
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
