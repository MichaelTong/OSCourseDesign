#include <dirent.h>
#include "global.h"
typedef struct line_editor{
    int tail;
    int cur;
}line_editor;

extern int tabFile(void);
int inHistory(int cur){//判断当前位置是否在History表中
    int start, endd;
    start = history.start;
    endd = history.end;
    if(start <= endd){
        if(start <= cur && cur <= endd)
            return 1;
    }
    else if(start > endd && endd >= 0){
            return 1;
    }
    else
        return 0;
    return 0;
}
extern int handleInBuff(void)//输入处理
{
    struct termios old_opt, opt;
    char ch;
    char* temp;
    int i = 0;
    int j = 0;
    int a1,a2,a3,a4;
    int x,y;

    tcgetattr(0, &old_opt);
    opt = old_opt;

    opt.c_lflag &= ~ECHO;
    opt.c_lflag &= ~ICANON;
    opt.c_lflag &= ~ISIG;
    memset(inputBuff, 0, 1000);
    history.cur = history.end;
    tcsetattr(0, TCSANOW, &opt); //设置
    while(1)
    {
        ch = getchar();

        if( ch == 27 ){
            getchar();
            if( (ch = getchar()) == 65 || ch == 66 ){
                tabflag=0;
                if(ch == 65 && inHistory(history.cur)){//上方向键
                    i = strlen(inputBuff);
                    memset(inputBuff, 0, 1000);
                    temp = history.cmds[history.cur];
                    history.cur--;
                    if(history.cur < 0){
                        history.cur += HISTORY_LEN;
                    }
                    strcpy(inputBuff, temp);
                    printf("\rxsh@%s>", get_current_dir_name());
                    for(j = 0; j < i; j++){
                        printf(" ");//覆盖掉自己
                    }
                    printf("\rxsh@%s>%s", get_current_dir_name(), inputBuff);
                    fflush(stdout);
                }
                else if(ch == 66 && inHistory((history.cur+2)%HISTORY_LEN)){//下方向键
                    history.cur+=2;
                    if(history.cur >= HISTORY_LEN){
                        history.cur %= HISTORY_LEN;
                    }
                    i = strlen(inputBuff);
                    memset(inputBuff, 0, 1000);
                    if(inHistory(history.cur)){
                        temp = history.cmds[history.cur];
                    }
                    else{
                        temp = "";
                        history.cur=history.end;
                    }
                    strcpy(inputBuff, temp);
                    printf("\rxsh@%s>", get_current_dir_name());
                    for(j = 0; j < i; j++){
                        printf(" ");//覆盖掉自己
                    }
                    printf("\rxsh@%s>%s", get_current_dir_name(), inputBuff);
                    fflush(stdout);
                }
                else if(ch == 66){
                    i = strlen(inputBuff);
                    memset(inputBuff, 0, 1000);
                    temp = "";
                    history.cur=history.end;
                    strcpy(inputBuff, temp);
                    printf("\rxsh@%s>", get_current_dir_name());
                    for(j = 0; j < i; j++){
                        printf(" ");//覆盖掉自己
                    }
                    printf("\rxsh@%s>%s", get_current_dir_name(), inputBuff);
                    fflush(stdout);
                }
            }
        }
        else if(ch == 127){//回格键
            tabflag=0;
            i = strlen(inputBuff);
            if(i != 0)
            {
                inputBuff[i - 1] = '\0';
                i--;
                printf("\rxsh@%s>", get_current_dir_name());
                for(j = 0;( j < i+1); j++)
                {
                    printf(" ");//覆盖掉自己

                }
                fflush(stdout);
                printf("\rxsh@%s>%s", get_current_dir_name(), inputBuff);
                fflush(stdout);
            }
        }
        else if(ch == 10){//回车结束
            tabflag=0;
            break;
        }
        else if(ch == 9){//tab键
            if(tabflag==0){
                tabFile();
            }
            if(recmdTop == -1){
                tabflag=0;
                continue;
            }
            else if(recmdTop == 0){//只有一个匹配项
                tabflag=0;
                i = strlen(inputBuff);
                for(j=i;j>=0;j--){
                    if(inputBuff[j]=='/'||inputBuff[j]==' ')
                        break;
                    inputBuff[j]='\0';
                }
                strcat(inputBuff, recommend[0].d_name);
                printf("\rxsh@%s>", get_current_dir_name());
                for(j = 0; j < i; j++){
                    printf(" ");//覆盖掉自己
                }
                printf("\rxsh@%s>%s", get_current_dir_name(), inputBuff);
            }
            else{
                if(tabflag==0){
                    tabflag=1;//有多个匹配项，按下两次tab才出现搜索结果
                    continue;
                }
                if(recmdTop==0)
                    continue;
                tabflag=0;
                for(a1=0,a2=1,a3=2,a4=3;a1<=recmdTop;a1+=4,a2+=4,a3+=4,a4+=4){
                    printf("\n%s\t",recommend[a1].d_name);
                    if(a2<=recmdTop)
                        printf("%s\t",recommend[a2].d_name);
                    if(a3<=recmdTop)
                        printf("%s\t",recommend[a3].d_name);
                    if(a4<=recmdTop)
                        printf("%s",recommend[a4].d_name);
                }
                fflush(stdout);
                printf("\nxsh@%s>%s", get_current_dir_name(), inputBuff);
                fflush(stdout);
            }
        }
        else if(ch==-1){
            continue;
        }
        else if(ch==3){
            printf("^C");
            memset(inputBuff, 0, 1000);
            break;
        }
        else{
            tabflag=0;
            i = strlen(inputBuff);
            printf("%c", ch);
            inputBuff[i] = ch; //命令储存
            i++;

        }
    }
    tcsetattr(0, TCSANOW, &old_opt); //恢复


    printf("\n");
    return strlen(inputBuff);
}
