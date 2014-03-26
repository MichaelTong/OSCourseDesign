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
                  �ʷ���������
****************************************************************/
int yylex(){
    int flag;
    char c;

	//�����ո��������Ϣ
    while(offset < len && (inputBuff[offset] == ' ' || inputBuff[offset] == '\t')){
        offset++;
    }

    flag = 0;
    while(offset < len){ //ѭ�����дʷ������������ս��
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
                  ������Ϣִ�к���
****************************************************************/
void yyerror(const char *s)
{
    printf("%s : command illegal!\n",s);
}

/****************************************************************
                  main������
****************************************************************/
int main(int argc, char** argv) {
    int i;
    Job *now;
    init(); //��ʼ������
    commandDone = 0;

    printf("xsh@%s>", get_current_dir_name()); //��ӡ��ʾ����Ϣ

    while(1){
        /*i = 0;
        while((c = getchar()) != '\n'){ //����һ������
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
        yyparse(); //�����﷨�����������ú�����yylex()�ṩ��ǰ����ĵ��ʷ���

        if(commandDone == 1){ //�����Ѿ�ִ����ɺ������ʷ��¼��Ϣ
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
