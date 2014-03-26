#include "global.h"
#include <dirent.h>
/** Defines and Macros */
#define MATCH      1
#define NOT_MATCH  0

/* 匹配一个字符的宏 */
#define MATCH_CHAR(c1,c2,ignore_case)  ( (c1==c2) || ((ignore_case==1) &&(tolower(c1)==tolower(c2))) )
void cleanTab(){//清除tab候选表
    while(recmdTop>=0){
        free(recommend[recmdTop].d_name);
        recommend[recmdTop].d_name=NULL;
        recommend[recmdTop].d_type=DT_UNKNOWN;
        recmdTop--;
    }
}

void sortrecommend()//排序
{
    int i,j;
    unsigned char t;
    char temp[20];
    for(i=0;i<recmdTop;i++)
    {
        for(j=0;j<recmdTop-i;j++)
        {
            if(strcmp(recommend[j].d_name,recommend[j+1].d_name)>0)
            {
                t=recommend[j].d_type;
                recommend[j].d_type=recommend[j].d_type;
                recommend[j+1].d_type=t;
                strcpy(temp,recommend[j].d_name);
                strcpy(recommend[j].d_name,recommend[j+1].d_name);
                strcpy(recommend[j+1].d_name,temp);
            }
        }
    }
}

extern int tabFile(void){//tab搜索文件
    struct dirent *ptr;
    char *temp1,*temp2;
    char path[255]={'\0'};
    char keyword[255]={'\0'};
    DIR *dir;
    int i;

    cleanTab();
    temp1=strrchr(inputBuff,' ');
    if(temp1==NULL)
        temp1=inputBuff;
    else
        temp1=temp1+1;
    temp2=strrchr(inputBuff,'/');
    if(temp2==NULL)
        temp2=temp1;
    strncpy(path,temp1,temp2-temp1);
    if(path[0]=='\0')
        strncpy(keyword,temp2,strlen(inputBuff)-(temp2-inputBuff));
    else
        strncpy(keyword,temp2+1,strlen(inputBuff)-(temp2-inputBuff));
    if(keyword[0]=='\0'){
        return 0;
    }
    if(path[0]!='\0'){//输入的项目中已经明确标定了文件路径
        if((dir=opendir(path))==NULL)
            return 0;
        while((ptr=readdir(dir))!=NULL){
            if(ptr->d_name[0]=='.')
                continue;
            if(strstr(ptr->d_name,keyword)==ptr->d_name){
                recommend[++recmdTop].d_name=(char*)malloc((strlen(ptr->d_name)+2)*sizeof(char));
                strcpy(recommend[recmdTop].d_name,ptr->d_name);
                recommend[recmdTop].d_type=ptr->d_type;
                if(ptr->d_type==DT_DIR)
                    strcat(recommend[recmdTop].d_name,"/");
            }
        }
        closedir(dir);
    }
    else{  //查找当前目录和ysh.conf文件中指定的目录，确定命令是否存在
        i=0;
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            if((dir=opendir(envPath[i]))==NULL)
                return 0;
            while((ptr=readdir(dir))!=NULL){
                if(ptr->d_name[0]=='.')
                    continue;
                if(strstr(ptr->d_name,keyword)==ptr->d_name){
                    recommend[++recmdTop].d_name=(char*)malloc((strlen(ptr->d_name)+1)*sizeof(char));
                    strcpy(recommend[recmdTop].d_name,ptr->d_name);
                    recommend[recmdTop].d_type=ptr->d_type;
                    if(ptr->d_type==DT_DIR)
                        strcat(recommend[recmdTop].d_name,"/");
                }
                i++;
            }
            closedir(dir);
        }
        if((dir=opendir("./"))==NULL)
            return 0;
        while((ptr=readdir(dir))!=NULL){
            if(ptr->d_name[0]=='.')
                continue;
            if(strstr(ptr->d_name,keyword)==ptr->d_name){
                recommend[++recmdTop].d_name=(char*)malloc((strlen(ptr->d_name)+1)*sizeof(char));
                strcpy(recommend[recmdTop].d_name,ptr->d_name);
                recommend[recmdTop].d_type=ptr->d_type;
                if(ptr->d_type==DT_DIR)
                    strcat(recommend[recmdTop].d_name,"/");
            }
        }
        closedir(dir);
        sortrecommend();
    }
    return (recmdTop+1);
}



/*---------------------------------------------------------------------------*/
/*  通配符匹配算法
 *        src      字符串
 *        pattern  含有通配符( * 或 ? 号)的字符串
 *        ignore_case 是否区分大小写，1 表示不区分,  0 表示区分
 *
 *  返回1表示 src 匹配 pattern，返回0表示不匹配
 */
extern int WildCharMatch(char *src, char *pattern, int ignore_case){
        int result;

        while (*src)
          {
                if (*pattern == '*')
                    {   /* 如果 pattern 的当前字符是 '*' */
                    	/* 如果后续有多个 '*', 跳过 */
                        while ((*pattern == '*') || (*pattern == '?'))
                              pattern++;

                        /* 如果 '*" 后没有字符了，则正确匹配 */
                        if (!*pattern) return MATCH;

                        /* 在 src 中查找一个与 pattern中'*"后的一个字符相同的字符*/
                        while (*src && (!MATCH_CHAR(*src,*pattern,ignore_case)))
                              src++;

                        /* 如果找不到，则匹配失败 */
                        if (!*src) return NOT_MATCH;

                        /* 如果找到了，匹配剩下的字符串*/
                        result = WildCharMatch (src, pattern, ignore_case);
                        /* 如果剩下的字符串匹配不上，但src后一个字符等于pattern中'*"后的一个字符 */
                        /* src前进一位，继续匹配 */
                        while ( (!result) && (*(src+1)) && MATCH_CHAR(*(src+1),*pattern,ignore_case) )
                           result = WildCharMatch (++src, pattern, ignore_case);

                        return result;

                    }
                else
                    {
                    	/* 如果pattern中当前字符不是 '*' */
                    	/* 匹配当前字符*/
                        if ( MATCH_CHAR(*src,*pattern,ignore_case) || ('?' == *pattern))
                          {
                            /* src,pattern分别前进一位，继续匹配 */
                            return WildCharMatch (++src, ++pattern, ignore_case);
                          }
                        else
                          {
                             return NOT_MATCH;
                          }
                    }
            }


       /* 如果src结束了，看pattern有否结束*/
       if (*pattern)
         {
            /* pattern没有结束*/
           if ( (*pattern=='*') && (*(pattern+1)==0) ) /* 如果pattern有最后一位字符且是'*' */
             return MATCH;
           else
             return NOT_MATCH;
         }
       else
         return MATCH;
}

extern void getRegex(int *b,int *e,char *path){//得到b,e之间有通配符最短的不含空格的路径串
    int i;
    int j;
    int flag=0;
    for(j=i=*b;i<*e;i++){
        if(path[i]=='/'||path[i]==' '){
            if(flag==0){
                j=i+1;
                continue;
            }
            else
                break;
        }
        if(path[i]=='*'||path[i]=='?'){
            flag=1;
        }
    }
    if(flag==0){
        *b=-1;
        *e=-1;
    }
    else{
        *b=j;
        *e=i;
    }
}

extern void regexChange(char *origin,char *newarg,unsigned char d_type){//用找到的匹配替换原来的参数
    char path[100];
    char regexx[100];
    struct dirent* ptr;
    int regexb,regexe;
    DIR *dir;
    if(origin[0]!='/'){
        strcpy(path,get_current_dir_name());
        strcat(path,"/");
    }
    regexb=0;
    regexe=strlen(origin);
    strcpy(newarg,origin);
    int old_copyend=0,copyend=0,old_e=0;
    if(strstr(origin,"*")!=NULL||strstr(origin,"?")!=NULL){
        while(1){
            getRegex(&regexb,&regexe,origin);
            if(regexb==-1)
                break;
            memset(regexx,0,100);
            strncpy(regexx,origin+regexb,regexe-regexb);
            strncat(path,newarg+copyend,regexb-old_e);
            copyend=regexb;
            dir = opendir(path);
            while((ptr=readdir(dir))!=NULL){
                if(ptr->d_type==d_type&&WildCharMatch(ptr->d_name,regexx,0)){
                    strcpy(newarg+copyend,ptr->d_name);
                    old_copyend=copyend;
                    copyend+=strlen(ptr->d_name);
                    strcpy(newarg+copyend,origin+regexe);
                    strcat(path,ptr->d_name);
                    break;
                }
            }
            old_e=regexe;
            regexb=regexe;
            regexe=strlen(origin);
        }
    }
}

extern int regexNum(char *origin){//得到通配符字符串的个数
    char path[100];
    char regexx[100];
    char newarg[100];
    struct dirent* ptr;
    int regexb,regexe;
    DIR *dir;
    int num=0;
    int flag=0;
    if(origin[0]!='/'){
        strcpy(path,get_current_dir_name());
        strcat(path,"/");
    }
    regexb=0;
    regexe=strlen(origin);
    strcpy(newarg,origin);
    int old_copyend=0,copyend=0,old_e=0;
    if(strstr(origin,"*")!=NULL||strstr(origin,"?")!=NULL){
        while(1){//循环往后，直到找不到为止
            getRegex(&regexb,&regexe,origin);
            if(regexb==-1)
                break;
            if(regexe==strlen(origin))
                flag=1;
            memset(regexx,0,100);
            strncpy(regexx,origin+regexb,regexe-regexb);
            strncat(path,newarg+copyend,regexb-old_e);
            copyend=regexb;
            dir = opendir(path);
            while((ptr=readdir(dir))!=NULL){
                if(WildCharMatch(ptr->d_name,regexx,0)){
                    strcpy(newarg+copyend,ptr->d_name);
                    old_copyend=copyend;
                    copyend+=strlen(ptr->d_name);
                    strcpy(newarg+copyend,origin+regexe);
                    strcat(path,ptr->d_name);
                    if(regexe!=strlen(origin))
                        break;
                    else
                        num++;
                }
            }
            old_e=regexe;
            regexb=regexe;
            regexe=strlen(origin);
        }
    }
    if(flag==0){
        if(access(newarg,F_OK)==0)
            num=0;
        else
            num=1;
    }
    if(num==0)
        printf("Can't find file of folder: %s.\n",origin);
    return num;
}

extern int regexNewArgs(char *origin, char **args,int *index){//找到匹配，更换原来的参数
    char path[100];
    char regexx[100];
    char newarg[100];
    struct dirent* ptr;
    int regexb,regexe;
    DIR *dir;
    int num=0;
    int flag=0;
    if(origin[0]!='/'){
        strcpy(path,get_current_dir_name());
        strcat(path,"/");
    }
    regexb=0;
    regexe=strlen(origin);
    strcpy(newarg,origin);
    int old_copyend=0,copyend=0,old_e=0;
    if(strstr(origin,"*")!=NULL||strstr(origin,"?")!=NULL){
        while(1){//寻找匹配，递归的
            getRegex(&regexb,&regexe,origin);
            if(regexb==-1)
                break;
            if(regexe==strlen(origin))
                flag=1;
            memset(regexx,0,100);
            strncpy(regexx,origin+regexb,regexe-regexb);
            strncat(path,newarg+copyend,regexb-old_e);
            copyend=regexb;
            dir = opendir(path);
            while((ptr=readdir(dir))!=NULL){
                if(WildCharMatch(ptr->d_name,regexx,0)){
                    if(regexe!=strlen(origin)){
                        strcpy(newarg+copyend,ptr->d_name);
                        old_copyend=copyend;
                        copyend+=strlen(ptr->d_name);
                        strcpy(newarg+copyend,origin+regexe);
                        strcat(path,ptr->d_name);
                        break;
                    }
                    else{
                        num++;
                        (*index)++;
                        args[*index]=(char*)malloc((strlen(path)+strlen(ptr->d_name)+1)*sizeof(char));
                        strcpy(args[*index],path);
                        strcat(args[*index],ptr->d_name);
                    }
                }
            }
            old_e=regexe;
            regexb=regexe;
            regexe=strlen(origin);
        }
    }
    if(flag==0){
        if(access(newarg,F_OK)==0)
            num=0;
        else{
            num=1;
            *index++;
            args[*index]=(char*)malloc((strlen(newarg)+1)*sizeof(char));
            strcpy(args[*index],newarg);
        }
    }
    return num;
}

