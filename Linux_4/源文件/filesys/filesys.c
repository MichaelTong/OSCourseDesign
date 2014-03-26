#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<ctype.h>
#include<time.h>
#include "filesys.h"


#define RevByte(low,high) ((high)<<8|(low))
#define RevWord(lowest,lower,higher,highest) ((highest)<< 24|(higher)<<16|(lower)<<8|lowest)

void clearfile()
{
    int offset;
    unsigned char ch[32]={0};
    offset=ROOTDIR_OFFSET;
    lseek(fd,offset,SEEK_SET);
    while(offset<DATA_OFFSET)
    {
        write(fd,ch,32);
        offset+=32;
    }
}

/*
*功能：打印启动项记录
*/
void ScanBootSector()
{
    unsigned char buf[SECTOR_SIZE];
    int ret,i;

    if((ret = read(fd,buf,SECTOR_SIZE))<0)
    {
        perror("read boot sector failed");
        return;
    }
    for(i = 0; i < 8; i++)
        bdptor.Oem_name[i] = buf[i+0x03];
    bdptor.Oem_name[i] = '\0';

    bdptor.BytesPerSector = RevByte(buf[0x0b],buf[0x0c]);
    bdptor.SectorsPerCluster = buf[0x0d];
    bdptor.ReservedSectors = RevByte(buf[0x0e],buf[0x0f]);
    bdptor.FATs = buf[0x10];
    bdptor.RootDirEntries = RevByte(buf[0x11],buf[0x12]);
    bdptor.LogicSectors = RevByte(buf[0x13],buf[0x14]);
    bdptor.MediaType = buf[0x15];
    bdptor.SectorsPerFAT = RevByte( buf[0x16],buf[0x17] );
    bdptor.SectorsPerTrack = RevByte(buf[0x18],buf[0x19]);
    bdptor.Heads = RevByte(buf[0x1a],buf[0x1b]);
    bdptor.HiddenSectors = RevByte(buf[0x1c],buf[0x1d]);


    printf("Oem_name \t\t%s\n"
           "BytesPerSector \t\t%d\n"
           "SectorsPerCluster \t%d\n"
           "ReservedSector \t\t%d\n"
           "FATs \t\t\t%d\n"
           "RootDirEntries \t\t%d\n"
           "LogicSectors \t\t%d\n"
           "MedioType \t\t%d\n"
           "SectorPerFAT \t\t%d\n"
           "SectorPerTrack \t\t%d\n"
           "Heads \t\t\t%d\n"
           "HiddenSectors \t\t%d\n",
           bdptor.Oem_name,
           bdptor.BytesPerSector,
           bdptor.SectorsPerCluster,
           bdptor.ReservedSectors,
           bdptor.FATs,
           bdptor.RootDirEntries,
           bdptor.LogicSectors,
           bdptor.MediaType,
           bdptor.SectorsPerFAT,
           bdptor.SectorsPerTrack,
           bdptor.Heads,
           bdptor.HiddenSectors);

    CLUSTER_SIZE = bdptor.SectorsPerCluster*SECTOR_SIZE;
    FAT_ONE_OFFSET = bdptor.ReservedSectors*SECTOR_SIZE;
    FAT_TWO_OFFSET = FAT_ONE_OFFSET + bdptor.SectorsPerFAT*SECTOR_SIZE;
    ROOTDIR_OFFSET = FAT_TWO_OFFSET + bdptor.SectorsPerFAT*SECTOR_SIZE;
    DATA_OFFSET = ROOTDIR_OFFSET + bdptor.RootDirEntries*DIR_ENTRY_SIZE;
}

/*日期*/
void findDate(unsigned short *year,
              unsigned short *month,
              unsigned short *day,
              unsigned char info[2])
{
    int date;
    date = RevByte(info[0],info[1]);

    *year = ((date & MASK_YEAR)>> 9 )+1980;
    *month = ((date & MASK_MONTH)>> 5);
    *day = (date & MASK_DAY);
}

/*时间*/
void findTime(unsigned short *hour,
              unsigned short *min,
              unsigned short *sec,
              unsigned char info[2])
{
    int time;
    time = RevByte(info[0],info[1]);

    *hour = ((time & MASK_HOUR )>>11);
    *min = (time & MASK_MIN)>> 5;
    *sec = (time & MASK_SEC) * 2;
}

/*
*文件名格式化，便于比较
*/
void FileNameFormat(unsigned char *name)
{
    unsigned char *p = name;
    while(*p!='\0')
        p++;
    p--;
    while(*p==' ')
        p--;
    p++;
    *p = '\0';
}

/*参数：entry，类型：struct Entry*
*返回值：成功，则返回偏移值；失败：返回负值
*功能：从根目录或文件簇中得到文件表项
*/
int GetEntry(struct Entry *pentry)
{
    int ret,i;
    int count = 0;
    unsigned char buf[DIR_ENTRY_SIZE], info[2];

    /*读一个目录表项，即32字节*/
    if( (ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
        perror("read entry failed");
    count += ret;

    if(buf[0]==0xe5 || buf[0]== 0x00)
        return -1*count;
    else
    {
        /*长文件名，忽略掉*/
        while (buf[11]== 0x0f)
        {
            if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
                perror("read root dir failed");
            count += ret;
        }

        /*命名格式化，主义结尾的'\0'*/
        for (i=0 ; i<=10; i++)
            pentry->short_name[i] = buf[i];
        pentry->short_name[i] = '\0';

        FileNameFormat(pentry->short_name);



        info[0]=buf[22];
        info[1]=buf[23];
        findTime(&(pentry->hour),&(pentry->min),&(pentry->sec),info);

        info[0]=buf[24];
        info[1]=buf[25];
        findDate(&(pentry->year),&(pentry->month),&(pentry->day),info);

        pentry->FirstCluster = RevByte(buf[26],buf[27]);
        pentry->size = RevWord(buf[28],buf[29],buf[30],buf[31]);

        pentry->readonly = (buf[11] & ATTR_READONLY) ?1:0;
        pentry->hidden = (buf[11] & ATTR_HIDDEN) ?1:0;
        pentry->system = (buf[11] & ATTR_SYSTEM) ?1:0;
        pentry->vlabel = (buf[11] & ATTR_VLABEL) ?1:0;
        pentry->subdir = (buf[11] & ATTR_SUBDIR) ?1:0;
        pentry->archive = (buf[11] & ATTR_ARCHIVE) ?1:0;

        return count;
    }
}

/*
*功能：显示当前目录的内容
*返回值：1，成功；-1，失败
*/
int fd_ls()
{
    int a,b;
        ls(1,&a,&b);
  //  fd_cd(temp);
}

int ls(int echo,int* dircount,int* filecount)
{
    int ret, offset,cluster_addr;
    struct Entry entry;
    *dircount=0;
    *filecount=0;
    if(echo)
    {
        if(curdir==NULL)
            printf("Root_dir\n");
        else
            printf("%s_dir\n",curdir->short_name);
        printf("\tname\tdate\t\t time\t\tcluster\tsize\t\tattr\n");
    }

    if(curdir==NULL)  /*显示根目录区*/
    {
        /*将fd定位到根目录区的起始地址*/
        if((ret= lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
            perror("lseek ROOTDIR_OFFSET failed");

        offset = ROOTDIR_OFFSET;

        /*从根目录区开始遍历，直到数据区起始地址*/
        while(offset < (DATA_OFFSET))
        {
            ret = GetEntry(&entry);

            offset += abs(ret);
            if(ret > 0)
            {
                if(echo)
                    printf("%12s\t"
                           "%d:%d:%d\t"
                           "%d:%d:%d   \t"
                           "%d\t"
                           "%d\t\t"
                           "%s\n",
                           entry.short_name,
                           entry.year,entry.month,entry.day,
                           entry.hour,entry.min,entry.sec,
                           entry.FirstCluster,
                           entry.size,
                           (entry.subdir) ? "dir":"file");
                if(entry.short_name[0]!=0x2e&&entry.subdir==1)
                    (*dircount)++;
                if(entry.short_name[0]!=0x2e&&entry.subdir==0)
                    (*filecount)++;
            }
        }
    }

    else /*显示子目录*/
    {
        int seed,next;
        next=curdir->FirstCluster;
        do
        {
            seed=next;
            next=GetFatCluster(seed);
            cluster_addr = DATA_OFFSET + (seed-2) * CLUSTER_SIZE ;
            if((ret = lseek(fd,cluster_addr,SEEK_SET))<0)
                perror("lseek cluster_addr failed");

            offset = cluster_addr;

            /*只读一簇的内容*/
            while(offset<cluster_addr +CLUSTER_SIZE)
            {
                ret = GetEntry(&entry);
                offset += abs(ret);
                if(ret > 0)
                {
                    if(echo)
                        printf("%12s\t"
                               "%d:%d:%d\t"
                               "%d:%d:%d   \t"
                               "%d\t"
                               "%d\t\t"
                               "%s\n",
                               entry.short_name,
                               entry.year,entry.month,entry.day,
                               entry.hour,entry.min,entry.sec,
                               entry.FirstCluster,
                               entry.size,
                               (entry.subdir) ? "dir":"file");
                    if(entry.short_name[0]!=0x2e&&entry.subdir==1)
                        (*dircount)++;
                    if(entry.short_name[0]!=0x2e&&entry.subdir==0)
                        (*filecount)++;
                }
            }
        }while(next!=0xffff);
    }
    return 0;
}


/*
*参数：entryname 类型：char
：pentry    类型：struct Entry*
：mode      类型：int，mode=1，为目录表项；mode=0，为文件
*返回值：偏移值大于0，则成功；-1，则失败
*功能：搜索当前目录，查找文件或目录项
*/
int ScanEntry (char *entryname,struct Entry *pentry,int mode)
{
    int ret,offset,i;
    int cluster_addr;
    char uppername[80];
    for(i=0; i< strlen(entryname); i++)
        uppername[i]= (entryname[i]);
    uppername[i]= '\0';
    /*扫描根目录*/
    if(curdir ==NULL)
    {
        if((ret = lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
            perror ("lseek ROOTDIR_OFFSET failed");
        offset = ROOTDIR_OFFSET;
        while(offset<DATA_OFFSET)
        {
            ret = GetEntry(pentry);
            offset +=abs(ret);

            if(pentry->subdir == mode && strcmp((char*)pentry->short_name,uppername)==0)

                return offset;
        }
        return -1;
    }

    /*扫描子目录*/
    else
    {
        cluster_addr = DATA_OFFSET + (curdir->FirstCluster -2)*CLUSTER_SIZE;
        if((ret = lseek(fd,cluster_addr,SEEK_SET))<0)
            perror("lseek cluster_addr failed");
        offset= cluster_addr;

        while(offset<cluster_addr + CLUSTER_SIZE)
        {
            ret= GetEntry(pentry);
            offset += abs(ret);
            if(pentry->subdir == mode && strcmp((char*)pentry->short_name,uppername)==0)
                return offset;
        }
        return -1;
    }
}

/*
把当前路径存入dir，然后返回层数
*/
int fd_cd(char *dir)
{
    char next[100],pre[1000];
    int i=0,j=0;
    int level=0,add=0;
    getcurpath(pre);
    do{
        i=0;
        while(dir[j]!=0&&dir[j]!='/')
        {
            next[i]=dir[j];
            i++;
            j++;
        }
        next[i]=0;
        if(dir[j]=='/'&&j==0)
        {
            next[0]='/';
            next[1]=0;
            j++;
        }
        else if(dir[j]=='/')
            j++;
        if((add=cd(next))==-2)
        {
            fd_cd(pre);
            return -10000;
        }
        level+=add;
    }while(dir[j]!=0);
    return level;
}

/*
*参数：dir，类型：char
*返回值：1，子目录；-1，父目录；-2，失败
*功能：改变目录到父目录或子目录
*/
int cd(char *dir)
{
    struct Entry *pentry;
    int ret;
    if(dir[0]==0)
        return 0;
    if(!strcmp(dir,"/"))
    {
        while(dirno)
        {
            free(curdir);
            curdir = fatherdir[dirno];
            dirno--;
        }
        return 1;
    }
    if(!strcmp(dir,"."))
    {
        return 0;
    }
    if(!strcmp(dir,"..") && curdir==NULL)
        return 0;
    /*返回上一级目录*/
    if(!strcmp(dir,"..") && curdir!=NULL)
    {
        curdir = fatherdir[dirno];
        dirno--;
        return -1;
    }
    pentry = (struct Entry*)malloc(sizeof(struct Entry));

    ret = ScanEntry(dir,pentry,1);
    if(ret < 0)
    {
        printf("no such dir\n");
        free(pentry);
        return -2;
    }
    dirno ++;
    fatherdir[dirno] = curdir;
    curdir = pentry;
    return 1;
}

/*
*参数：prev，类型：unsigned char
*返回值：下一簇
*在fat表中获得下一簇的位置
*/
unsigned short GetFatCluster(unsigned short prev)
{
    unsigned short next;
    int index;

    index = prev * 2;
    next = RevByte(fatbuf[index],fatbuf[index+1]);

    return next;
}

/*
*参数：cluster，类型：unsigned short
*返回值：void
*功能：清除fat表中的簇信息
*/
void ClearFatCluster(unsigned short cluster)
{
    int index;
    index = cluster * 2;

    fatbuf[index]=0x00;
    fatbuf[index+1]=0x00;

}


/*
*将改变的fat表值写回fat表
*/
int WriteFat()
{
    if(lseek(fd,FAT_ONE_OFFSET,SEEK_SET)<0)
    {
        perror("lseek failed");
        return -1;
    }
    if(write(fd,fatbuf,512*256)<0)
    {
        perror("read failed");
        return -1;
    }
    if(lseek(fd,FAT_TWO_OFFSET,SEEK_SET)<0)
    {
        perror("lseek failed");
        return -1;
    }
    if((write(fd,fatbuf,512*256))<0)
    {
        perror("read failed");
        return -1;
    }
    return 1;
}

/*
*读fat表的信息，存入fatbuf[]中
*/
int ReadFat()
{
    //if(lseek(fd,FAT_ONE_OFFSET,SEEK_SET)<0)
    if(lseek(fd,FAT_ONE_OFFSET,SEEK_SET)<0)
    {
        perror("lseek failed");
        return -1;
    }
    if(read(fd,fatbuf,512*256)<0)
    {
        perror("read failed");
        return -1;
    }
    return 1;
}


/*
*参数：filename，类型：char
*返回值：1，成功；-1，失败
*功能;删除当前目录下的文件
*/
int fd_df(char *dirname)
{
    char path[1000],name[100],pre[1000];
    int i,level,res;
    i=strlen(dirname)-1;
    getcurpath(pre);
    while(i>=0)
    {
        if(dirname[i]=='/')
            break;
        i--;
    }
    if(i>0)
    {
        strncpy(path,dirname,i);
        path[i]=0;
    }
    else
        path[0]=0;
    strcpy(name,dirname+i+1);
    level=fd_cd(path);
    if(level==-1)
        return -1;
    else{
        res=df(name);
        fd_cd(pre);
        return res;
    }

}

int df(char *filename)
{
    struct Entry *pentry;
    int ret;
    int res;

    pentry = (struct Entry*)malloc(sizeof(struct Entry));

    /*扫描当前目录查找文件*/
    ret = ScanEntry(filename,pentry,0);
    if(ret<0)
    {
        ret = ScanEntry(filename,pentry,1);
        if(ret<0)
        {
            printf("no such file or directory\n");
            free(pentry);
            return -1;
        }
        res=delete_dir(pentry,ret);
        free(pentry);
        return res;

    }
    res=delete_file(pentry,ret);
    free(pentry);
    return res;
}

int delete_file(struct Entry *pentry, int lastdir)
{
    unsigned short seed,next;
    unsigned char c;

    /*清除fat表项*/
    seed = pentry->FirstCluster;
    while((next = GetFatCluster(seed))!=0xffff)
    {
        ClearFatCluster(seed);
        seed = next;

    }

    ClearFatCluster( seed );

    /*清除目录表项*/
    c=0xe5;


    if(lseek(fd,lastdir-0x20,SEEK_SET)<0)
        perror("lseek fd_df failed");
    if(write(fd,&c,1)<0)
        perror("write failed");

    /*
    	if(lseek(fd,ret-0x40,SEEK_SET)<0)
    		perror("lseek fd_df failed");
    	if(write(fd,&c,1)<0)
    		perror("write failed");
    */;
    if(WriteFat()<0)
        exit(1);
    return 1;
}
/*
*参数：dirname，类型 char
*返回值：1，成； -1，失败
*功能；删除一个目录
*/
int delete_dir(struct Entry *pentry,int lastdir)
{
    unsigned short seed,next;
    int ret,cluster_addr,offset;
    struct Entry entry;
    unsigned char c;
    char ch[10];

    int dircount,filecount;
    fd_cd(pentry->short_name);
    ls(0,&dircount,&filecount);
    fd_cd("..");
    if(dircount!=0||filecount!=0)
    {
        printf("There exists %d dir(s) and %d file(s), are you sure to delete the dir?\n",dircount,filecount);
        while(1)
        {
            printf("(Y)es or (N)o:\n");
            scanf("%s",ch);

            if(ch[0]=='y'||ch[0]=='Y')
                break;
            else if(ch[0]=='n'||ch[0]=='N')
                return 0;
        }
    }
    next=pentry->FirstCluster;
    do
    {
        seed=next;
        next=GetFatCluster(seed);
        cluster_addr = DATA_OFFSET + (seed-2) * CLUSTER_SIZE ;

        offset = cluster_addr;
        /*读一簇的内容*/
        while(offset<cluster_addr +CLUSTER_SIZE)
        {
            if((ret = lseek(fd,offset,SEEK_SET))<0)
                perror("lseek offset failed");
            ret = GetEntry(&entry);
            offset += abs(ret);
            if(ret > 0)
            {
                if(entry.subdir==1&&entry.short_name[0]!=0x2e)
                    delete_dir(&entry,offset);
                if(entry.subdir!=1&&entry.short_name[0]!=0x2e)
                {
                    unsigned short seed1,next1;
                    /*清除fat表项*/
                    seed1 = entry.FirstCluster;
                    while((next1 = GetFatCluster(seed1))!=0xffff)
                    {
                        ClearFatCluster(seed1);
                        seed1 = next1;
                    }
                    ClearFatCluster( seed1 );

                    c=0xe5;
                    if(lseek(fd,offset-0x20,SEEK_SET)<0)
                        perror("lseek fd_df failed");
                    if(write(fd,&c,1)<0)
                        perror("write failed");
                    if(WriteFat()<0)
                        exit(1);
                }
            }
        }
    }
    while(next!=0xffff);

    unsigned short seed1,next1;
    /*清除fat表项*/
    seed1 = pentry->FirstCluster;
    while((next1 = GetFatCluster(seed1))!=0xffff)
    {
        ClearFatCluster(seed1);
        seed1 = next1;
    }
    ClearFatCluster( seed1 );
    c=0xe5;
    if(lseek(fd,lastdir-0x20,SEEK_SET)<0)
        perror("lseek fd_df failed");
    if(write(fd,&c,1)<0)
        perror("write failed");

    /*
    	if(lseek(fd,ret-0x40,SEEK_SET)<0)
    		perror("lseek fd_df failed");
    	if(write(fd,&c,1)<0)
    		perror("write failed");
    */
    if(WriteFat()<0)
        exit(1);
    return 1;
}
int fd_cf(char *dirname,int size)
{
    char path[1000],name[100],pre[1000];
    int i,level,res;
    i=strlen(dirname)-1;
    getcurpath(pre);
    while(i>=0)
    {
        if(dirname[i]=='/')
            break;
        i--;
    }
    if(i>0)
    {
        strncpy(path,dirname,i);
        path[i]=0;
    }
    else
        path[0]=0;
    strcpy(name,dirname+i+1);
    level=fd_cd(path);
    if(level==-10000)
        return -1;
    else{
        res=cf(name,size,0);
        fd_cd(pre);
        return res;
    }
}
int fd_mkdir(char *dirname)
{
    char path[1000],name[100],pre[1000];
    int i,level,res;
    i=strlen(dirname)-1;
    getcurpath(pre);
    while(i>=0)
    {
        if(dirname[i]=='/')
            break;
        i--;
    }
    if(i>0)
    {
        strncpy(path,dirname,i);
        path[i]=0;
    }
    else
        path[0]=0;
    strcpy(name,dirname+i+1);
    level=fd_cd(path);
    if(level==-10000)
        return -1;
    else{
        res=cf(name,512*32,1);
        fd_cd(pre);
        return res;
    }
}
/*
*参数：filename，类型：char，创建文件的名称
size，    类型：int，文件的大小
mode,       int,    file(0) or directory(1)
*返回值：1，成功；-1，失败
*功能：在当前目录下创建文件
*/
int cf(char *filename,int size,int mode)
{
    struct Entry *pentry;
    int ret,i=0,cluster_addr,offset;
    unsigned short cluster,clusterno[100];
    unsigned char c[DIR_ENTRY_SIZE];
    int index,clustersize;
    unsigned char buf[DIR_ENTRY_SIZE];


    //获取时间
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );


    pentry = (struct Entry*)malloc(sizeof(struct Entry));

    clustersize = (size / (CLUSTER_SIZE));

    if(size % (CLUSTER_SIZE) != 0)
        clustersize ++;

    //扫描根目录，是否已存在该文件名
    ret = ScanEntry(filename,pentry,mode);
    if (ret<0)
    {
        /*查询fat表，找到空白簇，保存在clusterno[]中*/
        for(cluster=2; cluster<1000; cluster++)
        {
            index = cluster *2;
            if(fatbuf[index]==0x00&&fatbuf[index+1]==0x00)
            {
                clusterno[i] = cluster;

                i++;
                if(i==clustersize)
                    break;

            }

        }

        /*在fat表中写入下一簇信息*/
        for(i=0; i<clustersize-1; i++)
        {
            index = clusterno[i]*2;

            fatbuf[index] = (clusterno[i+1] &  0x00ff);
            fatbuf[index+1] = ((clusterno[i+1] & 0xff00)>>8);


        }
        /*最后一簇写入0xffff*/
        index = clusterno[i]*2;
        fatbuf[index] = 0xff;
        fatbuf[index+1] = 0xff;

        if(curdir==NULL)  /*往根目录下写文件*/
        {

            if((ret= lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
                perror("lseek ROOTDIR_OFFSET failed");
            offset = ROOTDIR_OFFSET;
            while(offset < DATA_OFFSET)
            {
                if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
                    perror("read entry failed");

                offset += abs(ret);
                //ignore long name
                if(buf[0]!=0xe5&&buf[0]!=0x00)
                {
                    while(buf[11] == 0x0f)
                    {
                        if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
                            perror("read root dir failed");
                        offset +=abs(ret);
                    }
                }
                /*找出空目录项或已删除的目录项*/
                else
                {
                    offset = offset-abs(ret);
                    break;
                }

            }
        }
        else
        {
            int next=curdir->FirstCluster;
            int seed;
            int find=0;
            do
            {
                seed=next;
                cluster_addr = (seed -2 )*CLUSTER_SIZE + DATA_OFFSET;
                if((ret= lseek(fd,cluster_addr,SEEK_SET))<0)
                    perror("lseek cluster_addr failed");
                offset = cluster_addr;
                next=GetFatCluster(seed);
                while(offset < cluster_addr + CLUSTER_SIZE)
                {
                    if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
                        perror("read entry failed");

                    offset += abs(ret);

                    if(buf[0]!=0xe5&&buf[0]!=0x00)
                    {
                        while(buf[11] == 0x0f)
                        {
                            if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
                                perror("read root dir failed");
                            offset +=abs(ret);
                        }
                    }
                    else
                    {
                        offset = offset - abs(ret);
                        find=1;
                        break;
                    }
                }
                if(find)
                    break;
            }
            while(next!=0xffff);
            //dir file not big enough
            if(!find)
            {
                int k,ind;
                for(k=2; k<1000; k++)
                {
                    ind = k *2;
                    if(fatbuf[ind]==0x00&&fatbuf[ind+1]==0x00)
                    {
                        break;
                    }
                }
                fatbuf[seed*2] = (k &  0x00ff);
                fatbuf[seed*2+1] = ((k & 0xff00)>>8);
                fatbuf[ind] = 0xff;
                fatbuf[ind+1] = 0xff;
                offset = (k -2 )*CLUSTER_SIZE + DATA_OFFSET;
            }
        }
        //write dir
        for(i=0; i<=strlen(filename); i++)
        {
            c[i]=filename[i];
        }
        for(; i<=10; i++)
            c[i]=' ';

        c[11] = mode?0x10:0x00;

        /*写第一簇的值*/
        c[26] = (clusterno[0] &  0x00ff);
        c[27] = ((clusterno[0] & 0xff00)>>8);

        //size = (mode:0?size);
        /*写文件的大小*/
        c[28] = (size &  0x000000ff);
        c[29] = ((size & 0x0000ff00)>>8);
        c[30] = ((size& 0x00ff0000)>>16);
        c[31] = ((size& 0xff000000)>>24);

        /*写时间*/
        c[22] = ((timeinfo->tm_min << 5) | (timeinfo->tm_sec/2)) & 0x000000ff;
        c[23] = (((timeinfo->tm_min << 5) | (timeinfo->tm_hour << 11)) & 0x0000ff00)>>8;
        /*写日期*/
        c[24] = ((timeinfo->tm_mon + 1) << 5) | timeinfo->tm_mday;
        c[25] = (((timeinfo->tm_year-80)<<1)|(timeinfo->tm_mon + 1) >>3);

        if(lseek(fd,offset,SEEK_SET)<0)
            perror("lseek fd_cf failed");
        if(write(fd,c,DIR_ENTRY_SIZE)<0)
            perror("write failed");

        free(pentry);
        if(WriteFat()<0)
            exit(1);
        //write directory content
        if(mode)
        {
            offset=(clusterno[0]-2)*CLUSTER_SIZE + DATA_OFFSET;
            //.
            c[0]=0x2e;
            for(i=1; i<=10; i++)
                c[i]=' ';
            c[11] = 0x10;
            /*写第一簇的值*/
            c[26] = (clusterno[0] &  0x00ff);
            c[27] = ((clusterno[0] & 0xff00)>>8);
            /*写文件的大小*/
            c[28] = (0 &  0x000000ff);
            c[29] = ((0 & 0x0000ff00)>>8);
            c[30] = ((0 & 0x00ff0000)>>16);
            c[31] = ((0 & 0xff000000)>>24);
            /*写时间*/
            c[22] = ((timeinfo->tm_min << 5) | (timeinfo->tm_sec/2)) & 0x000000ff;
            c[23] = (((timeinfo->tm_min << 5) | (timeinfo->tm_hour << 11)) & 0x0000ff00)>>8;
            /*写日期*/
            c[24] = ((timeinfo->tm_mon + 1) << 5) | timeinfo->tm_mday;
            c[25] = (((timeinfo->tm_year-80)<<1)|(timeinfo->tm_mon + 1) >>3);

            if(lseek(fd,offset,SEEK_SET)<0)
                perror("lseek fd_cf failed");
            if(write(fd,&c,DIR_ENTRY_SIZE)<0)
                perror("write failed");
            //..
            c[0]=0x2e;
            c[1]='.';
            for(i=2; i<=10; i++)
                c[i]=' ';
            c[11] = 0x10;
            if(curdir)
            {
                c[26]=(curdir->FirstCluster & 0x00ff);
                c[27]=((curdir->FirstCluster & 0x00ff)>>8);
            }
            else
            {
                c[26]=0x00;
                c[27]=0x00;
            }
            /*写文件的大小*/
            c[28] = ((32*512) &  0x000000ff);
            c[29] = (((32*512) & 0x0000ff00)>>8);
            c[30] = (((32*512) & 0x00ff0000)>>16);
            c[31] = (((32*512) & 0xff000000)>>24);
            /*写时间*/
            c[22] = ((timeinfo->tm_min << 5) | (timeinfo->tm_sec/2)) & 0x000000ff;
            c[23] = (((timeinfo->tm_min << 5) | (timeinfo->tm_hour << 11)) & 0x0000ff00)>>8;
            /*写日期*/
            c[24] = ((timeinfo->tm_mon + 1) << 5) | timeinfo->tm_mday;
            c[25] = (((timeinfo->tm_year-80)<<1)|(timeinfo->tm_mon + 1) >>3);
            if(write(fd,&c,DIR_ENTRY_SIZE)<0)
                perror("write failed");
        }
        return 1;
    }
    else
    {
        printf("This filename is exist\n");
        free(pentry);
        return -1;
    }
    return 1;
}

int fd_cf_str(char *file_name,char *str)
{
    char path[1000],name[100],pre[1000];
    int i,level,res,size;
    int addoffset;
    int cluster,index;
    struct Entry pentry;
    i=strlen(file_name)-1;
    getcurpath(pre);
    while(i>=0)
    {
        if(file_name[i]=='/')
            break;
        i--;
    }
    if(i>=0)
    {
        strncpy(path,file_name,i);
        path[i]=0;
    }
    else
        path[0]=0;
    strcpy(name,file_name+i+1);
    level=fd_cd(path);
    if(level==-10000)
        return -1;
    else{
        size=strlen(str);
        res=cf(name,size,0);
        if(ScanEntry(name,&pentry,0)<0)
        {
            printf("scanentry failed\n");
            return -1;
        }
        addoffset=(pentry.FirstCluster-2)*CLUSTER_SIZE+DATA_OFFSET;
        if(lseek(fd,addoffset,SEEK_SET)<0)
        {
            perror("seek failed\n");
            return -1;
        }
        if(write(fd,(unsigned char*)str,size)<=0)
        {
            perror("write failed");
            return -1;
        }
        fd_cd(pre);
        return res;
    }
}

void do_usage()
{
    printf("please input a command, including followings:\n \
           \tls\t\t\tlist all files\n \
           \tcd <dir>\t\tchange directory\n \
           \tcf <filename> <size>\tcreate a file\n \
           \tcf <filename> -m\tput in the content right away \
           \t\tmkdir <dirname>\t\tcreate a directory\n \
           \tdf <file>\t\tdelete a file\n \
           \texit\t\t\texit this system\n");
}
void getcurpath(char* temp)
{
    char buf[100];
    int i=2;
    memset(temp,0,1000);
    for(;i<=dirno;i++)
    {
        sprintf(buf,"/%s",fatherdir[i]->short_name);
        strcat(temp,buf);
    }
    sprintf(buf,"/");
    strcat(temp,buf);
    if(curdir){
        sprintf(buf,"%s",curdir->short_name);
        strcat(temp,buf);
    }
    strcat(temp,"");
}

void printpath()
{
    int i=2;
    for(;i<=dirno;i++)
    {
        printf("/%s",fatherdir[i]->short_name);
    }
    printf("/");
    if(curdir)
        printf("%s",curdir->short_name);
    printf(">");
}

int main()
{
    char input[10];
    int size=0;
    char name[12],ch;
    char tempstr[256];
    if((fd = open(DEVNAME,O_RDWR))<0)
    {
        perror("open failed");
        return -1;
    }
    ScanBootSector();
    //clearfile();
    if(ReadFat()<0)
        exit(1);
    do_usage();
    while (1)
    {
        printpath();
        scanf("%s",input);

        if (strcmp(input, "exit") == 0)
            break;
        else if (strcmp(input, "ls") == 0)
        {
            fd_ls();
        }
        else if(strcmp(input, "cd") == 0)
        {
            scanf("%s", name);
            fd_cd(name);
        }
        else if(strcmp(input, "df") == 0)
        {
            scanf("%s", name);
            fd_df(name);
        }
        else if(strcmp(input, "cf") == 0)
        {
            scanf("%s", name);
            scanf("%s", input);
            if(strcmp(input,"-m")==0)
            {
                printf("please put in the content ending with Ctrl+D\n");
                size = 0;
                while((ch=getchar())!=EOF)
                {
                    tempstr[size] = ch;
                    size++;
                }
                tempstr[size]='\0';
                fd_cf_str(name,tempstr);
            }
            else
            {
                size = atoi(input);
                fd_cf(name,size);
            }
        }
        else if(strcmp(input,"mkdir")==0)
        {
            scanf("%s", name);
            fd_mkdir(name);
        }
        else
            do_usage();
    }

    return 0;
}



