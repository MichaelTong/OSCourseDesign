%{
    #include "global.h"

    int yylex(void);
    void yyerror (const char *s);
    int offset, len, commandDone;
%}

%token STRING

%%
line            :	/* empty */
			|command			{ execute(isSimple); commandDone=1; return 1;}
;

command         :  	fgCommand
                    	|fgCommand'&'
;

fgCommand       :   	simpleCmd
                    	|compondCmd
;

simpleCmd       :   	progInvocation inputRedirect outputRedirect
;

compondCmd      :   	progInvocation inputRedirect'|'nextCmd {isSimple=0;}
;

nextCmd         :   	progInvocation outputRedirect
                    	|progInvocation'|'nextCmd {isSimple=0;}
;

progInvocation  :   	STR args
;

inputRedirect   :   	/* empty */
                    	|'<' STR
;

outputRedirect  :   	/* empty */
                    	|'>' STR
			|'>''>' STR
;

args            :   	/* empty */
                    	|args STR
;

STR		:	rex STRING rex
			|rex '.' rex
;

rex		:	'*'|'?'|
;

%%

/****************************************************************
                  词法分析函数
****************************************************************/
int yylex(){
    int flag;
    char c;

	//跳过空格等无用信息
    while(offset < len && (inputBuff[offset] == ' ' || inputBuff[offset] == '\t')){
        offset++;
    }

    flag = 0;
    while(offset < len){ //循环进行词法分析，返回终结符
        c = inputBuff[offset];

        if(c == ' ' || c == '\t'){
            offset++;
            return STRING;
        }

        if(c == '<' || c == '>' || c == '&'){
            if(flag == 1){
                flag = 0;
                return STRING;
            }
            offset++;
            return c;
        }

        flag = 1;
        offset++;
    }

    if(flag == 1){
        return STRING;
    }else{
        return 0;
    }
}

/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror(const char *s)
{
    printf("%s : command illegal!\n",s);
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    Job *now;
    init(); //初始化环境
    commandDone = 0;

    printf("xsh@%s>", get_current_dir_name()); //打印提示符信息

    while(1){
        /*i = 0;
        while((c = getchar()) != '\n'){ //读入一行命令
            inputBuff[i++] = c;
        }
        inputBuff[i] = '\0';*/

        nredirect = 0;
        isSimple = 1;
        i=handleInBuff();
        int x;
        for(x=0;x<strlen(inputBuff);x++){
            if(inputBuff[x]=='|'){
                isSimple=0;
                nredirect++;
            }
        }
        len = i;
        offset = 0;
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号

        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
        if(bflag==1){
        	printf("xsh@%s>", get_current_dir_name());
            fflush(stdout);
		}
		else if(bflag==0){
			bflag=2;
		}
		else if(bflag==2){
            for(i=1,now = fHead; now != NULL; i++){
                printf("[%d]\t%d\t%s\t%s\n", i, now->pid, now->state, now->cmd);
                now=now->next;
                free(fHead);
                fHead=now;
            }
			printf("xsh@%s>", get_current_dir_name());
            fflush(stdout);
			bflag=1;
		}
    }
    return (EXIT_SUCCESS);
}
