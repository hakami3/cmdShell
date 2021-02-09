#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <dirent.h>

pid_t pid;
struct sigaction act;

typedef struct list
{
	char *value;
	struct list *prev;
	struct list *next;
}H_list; // 히스토리 목록 리스트

H_list* hishead; // 히스토리 헤더
H_list* histail; // 히스토리 꼬리

struct CMD {
	char *name;
	char *desc;
	int(*cmd)(int argc, char *argv[]);
};

int cmd_cd(int argc, char *argv[]);
int cmd_pwd(int argc, char *argv[]);
int cmd_exit(int argc, char *argv[]);
void add_list(char *command); // 히스토리 추가
void exe_list(char* command); // 히스토리 실행
void cmd_history(int argc, int *argv[]);
int cmd_redirect(int fd, int num, char *cmdTokens[], int TokenNum);
int cmd_pipe(int argc, char *argv[]);

int cmdProcessing(void);
struct CMD builtin[] = {
	{ "cd", "작업 디렉토리 이동",cmd_cd },
	{ "pwd", "현재 작업 디렉토리",cmd_pwd },
	{ "exit", "쉘 나가기",cmd_exit },
	{ "history", "명령어 입력 기록",cmd_history}
};

const int builtins = 4;

void handler(int ignum)
{
	printf("\n");
	kill(0, SIGINT);
}
int main(void)
{
	int isExit = 0;
	act.sa_handler = handler;
	sigaction(SIGINT, &act, NULL);

	while (!isExit) {
		isExit = cmdProcessing();

	}
	fputs("My Shell을 종료합니다", stdout);

	return 0;
}

#define STR_LEN 1024
#define MAX_TOKENS 128


int cmdProcessing(void)
{
	char cmdLine[STR_LEN];
	char *cmdTokens[MAX_TOKENS];
	char delim[] = " \t\n\r";
	char *token;
	int tokenNum=0;
	int exitCode = 0;
	int status;
	char allcmd[STR_LEN] = "";
	int check = 0;
	int fd;


	fputs("[mysh v0.1] $ ", stdout);
	fgets(cmdLine, STR_LEN, stdin);

	tokenNum = 0;
	token = strtok(cmdLine, delim);

	while (token) {
		cmdTokens[tokenNum++] = token;
		token = strtok(NULL, delim);
	}
	cmdTokens[tokenNum] = NULL;
	if (tokenNum == 0)
		return exitCode;

	if (tokenNum > 2)
	{
		if (strcmp(cmdTokens[tokenNum - 2], "<") == 0) //표준입력을 파일에 입력
		{
			if (access(cmdTokens[tokenNum - 1], X_OK) == -1)
			{
				fprintf(stderr, "실행 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}

			if (access(cmdTokens[tokenNum - 1], R_OK) == -1)
			{
				fprintf(stderr, "읽기 권한이 없습니다. - %s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}
			if (access(cmdTokens[tokenNum - 1], W_OK) == -1)
			{
				fprintf(stderr, "쓰기 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}
			if (access(cmdTokens[tokenNum - 1], F_OK) == -1)
			{
				fprintf(stderr, "파일이 존재하지 않습니다. -%s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}

			if ((fd = open(cmdTokens[tokenNum - 1], O_RDONLY)) == -1)
				return -1;
			check = 1;
			cmd_redirect(fd, check, cmdTokens, tokenNum);
			close(fd);

		}
		else if (strcmp(cmdTokens[tokenNum - 2], ">") == 0)  //표준 출력을 파일에 출력
		{
			if (access(cmdTokens[tokenNum - 1], X_OK) == -1)
			{
				fprintf(stderr, "실행 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}

			if (access(cmdTokens[tokenNum - 1], R_OK) == -1)
			{
				fprintf(stderr, "읽기 권한이 없습니다. - %s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}
			if (access(cmdTokens[tokenNum - 1], W_OK) == -1)
			{
				fprintf(stderr, "쓰기 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}
			if (access(cmdTokens[tokenNum - 1], F_OK) == -1)
			{
				fprintf(stderr, "파일이 존재하지 않습니다. -%s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}

			if ((fd = open(cmdTokens[tokenNum - 1], O_RDONLY)) == -1)
				return -1;
			check = 2;
			cmd_redirect(fd, check, cmdTokens, tokenNum);
			close(fd);
		}
		else if (strcmp(cmdTokens[tokenNum - 2], ">>") == 0)  //파일에 내용 추가
		{
			if (access(cmdTokens[tokenNum - 1], X_OK) == -1)
			{
				fprintf(stderr, "실행 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}

			if (access(cmdTokens[tokenNum - 1], R_OK) == -1)
			{
				fprintf(stderr, "읽기 권한이 없습니다. - %s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}
			if (access(cmdTokens[tokenNum - 1], W_OK) == -1)
			{
				fprintf(stderr, "쓰기 권한이 없습니다. -%s\n", cmdTokens[tokenNum - 1]);
			}
			if (access(cmdTokens[tokenNum - 1], F_OK) == -1)
			{
				fprintf(stderr, "파일이 존재하지 않습니다. -%s\n", cmdTokens[tokenNum - 1]);
				return 0;
			}

			if ((fd = open(cmdTokens[tokenNum - 1], O_RDONLY)) == -1)
				return -1;
			check = 3;
			cmd_redirect(fd, check, cmdTokens, tokenNum);
			close(fd);
		}
		else if (strcmp(cmdTokens[tokenNum - 2], "|") == 0)
		{
			cmd_pipe(tokenNum, cmdTokens);
			cmdTokens[tokenNum - 1] = NULL;
			cmdTokens[tokenNum - 2] = NULL;
			cmdTokens[tokenNum - 3] = NULL;
			tokenNum = tokenNum - 3;
		}
	}


	add_list(cmdLine);		// 히스토리 추가
	if (cmdLine[0] == '!') //히스토리 목록 실행
	{ 
		exe_list(cmdLine);
	}

	
	for (int i = 0; i<builtins; ++i) {

		if (strcmp(cmdTokens[0], builtin[i].name) == 0)
		{
			return builtin[i].cmd(tokenNum, cmdTokens);
		}
	}
	pid = fork();

	if (pid>0) 
	{
		wait(&status);
	}
	if (pid == 0)
	{
		act.sa_handler = SIG_DFL;
		sigaction(SIGINT, &act, NULL);
		execvp(cmdTokens[0], cmdTokens);

		exit(0);
	}
	return exitCode;
}

int cmd_cd(int argc, char *argv[])
{
	struct stat finfo1;
	mode_t modes1;
	stat(argv[1], &finfo1);
	modes1 = finfo1.st_mode;


	if (argc == 2)
	{
		if (access(argv[1], F_OK) >= 0 && S_ISDIR(modes1) == 1)
			chdir(argv[1]);
		else if (S_ISDIR(modes1)<1 && access(argv[1], F_OK) >= 0)
			printf("디렉터리 형태가 아닙니다 \n");
		else if (access(argv[1], F_OK)<0)
			printf("디렉터리가 없습니다 \n");
	}
	else if (argc == 1)
	{
		chdir(getenv("HOME"));
	}
	else if (argc>2)
	{
		printf("인자를 적절히 입력하여 주세요 \n");
	}
	return 0;
}
int cmd_pwd(int argc, char*argv[])
{
	char path[256];
	getcwd(path, 256);
	printf("%s \n", path);

	return 0;
}
int cmd_exit(int argc, char*argv[])
{
	return 1;
}


void add_list(char *command) // 히스토리 추가
{ 
	H_list* t;
	H_list* h;

	t = (H_list*)malloc(sizeof(H_list));
	t->value = (char*)malloc(strlen(command) + 1);
	strcpy(t->value, command);
	t->prev = NULL;
	t->next = NULL;	
	if (histail == NULL)// 이중 링크드 리스트 구조로 추가
	{
		histail = t;
		hishead = t;
	}
	else
	{
		t->prev = histail;
		histail->next = t;
		histail = histail->next;
	}

	h = hishead;

}

void cmd_history(int argc,int *argv[])
{ // 히스토리 출력
	int num = 0;
	H_list* t = hishead;

	while (1)
	{
		printf(" %d %s\n", num + 1, t->value);
		if (t->next == NULL)
		{
			break;
		}
		t = t->next;
		num++;
	}
}

void exe_list(char *command)		//히스토리 목록 실행함
{ 
	char* t_command;	// 명령을 저장하기 위한 포인터배열
	char num[100] = { 0 };	//100개 까지 저장

	int flag = 1;
	int sel = 0;
	int n = 0, i = 0;

	H_list* t;

	t = hishead;

	t_command = (char*)malloc(strlen(command) + 1);
	strcpy(t_command, command);

	while (t_command[n] != '\0')
	{
		if (t_command[n] == '!')
		{
			n++;
			break;
		}
		n++;
	}
	// !을 파악
	if (t_command[n] == '!')
	{	
		if (t_command[n+1] == '\0' || t_command[n+1] ==' ' ) // 마지막 명령어 실행
		{ 

			strcpy(command, histail->value);
			flag = 0;
		}
	}
	else
	{
		while (t_command[n] != '\0') // 숫자 확인
		{ 
			if (t_command[n] >= '0' && t_command[n] <= '9')
			{
				num[i] = t_command[n];
				i++;
				n++;
			}
			else
			{
				flag = 0;
				break;
			}
		}
	}
	if (flag == 1)
	{
		sel = atoi(num);

		for (i = 1; i<sel; i++)
		{
			t = t->next;
		}
		strcpy(command, t->value);
	}
}

int cmd_pipe(int argc, char *argv[]) {

	char *p_p[2]; // 파이프 이전의 분리된 명령어 포인터배열 
	char *c_p[2];	//파이프 이후의 분리된 명령어 포인터배열
	int fd[2];	//입력, 출력을 위한 조건변수
	pid_t pid1, pid2;

	p_p[0] = argv[0];	//파이프라인 이전의 명령어
	p_p[1] = NULL;	//명령어 이후의 공백란

	c_p[0] = argv[2];		//파이프라인 이후의 명령어
	c_p[1] = NULL;		
	
	if (pipe(fd) == 1)
	{
		return -1;
	}
	pid1 = fork();	//부모 프로세스
	switch (pid1)
	{
	case 0: //호출된 프로세스
			close(fd[0]);
			if (fd[1] != 1) {		//프로세스의 출력이 아닐시
				dup2(fd[1], 1);
				close(fd[1]);
			}
			execvp(p_p[0], p_p);		//첫번째 명령어와 주소 입력
	case -1:	
			perror("Error!");
			exit(1);
	}
	pid2 = fork();	// 자식 프로세스
	switch (pid2)
	{
	case 0:
			close(fd[1]);
			if (fd[0] != 0) {
				dup2(fd[0], 0);
				close(fd[0]);
			}
			execvp(c_p[0], c_p);
	case -1:
			perror("Error!");
			exit(1);
	}

	close(fd[0]);
	close(fd[1]);

	while (wait(NULL) != -1);		// -1 오류가 아닐때까지 통신
	return 0;
}

int cmd_redirect(int fd, int check, char *cmdTokens[], int tokenNum)
{

	if (check >1)
	{
		dup2(1, 700);
		dup2(fd, 1);
	}
	else if (check == 1)
	{
		dup2(0, 600);  
		dup2(fd, 0);
	}

	cmdTokens[tokenNum - 1] = NULL;		// 명령어 사이의 스페이스를 null로 함
	cmdTokens[tokenNum - 2] = NULL;
	tokenNum = tokenNum - 2;

	return 0;
}
