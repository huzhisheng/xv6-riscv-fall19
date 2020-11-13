#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// 参考sh.c

#define MAXARGS 50

int fork1(void); // Fork but panics on failure.
void panic(char *);
void run_single_cmd(int argc, char **args);
int try_to_redir(int argc, char **args);

int getcmd(char *buf, int nbuf)		//从sh.c中借鉴的函数，用来获取输入
{
	fprintf(2, "@ ");
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}

/*
* 将输入的一串字符串看作是 "single_cmd | single_cmd | ..." 
* 即多个single_cmd通过pipe连接起来, 因此把不包含pipe的cmd看作是单个cmd, 把包含pipe的cmd看作是多个 “单个cmd” 的组合体
* 将pipe_cmd看作是 "single_cmd | pipe_cmd"以此递归下去, 函数每次处理一个single_cmd即可
*/
void run_pipe_cmd(int argc, char **args)	
{
	int p[2];
	int i = 0;
	for (i = 0; i < argc; i++)
	{
		if (strcmp(args[i], "|") == 0)
		{
			break;
		}
	}
	
	//fprintf(2,"check i in pipe, %d, %d\n",i,argc);
	if (i != argc)
	{
		args[i] = 0;
		if (pipe(p) < 0)
		{
			panic("pipe");
		}
		
		if (fork1() == 0)	//处理pipe管道, 和之前的pipe实验做法相同, 一定要关闭多余的通道
		{
			close(1);
			dup(p[1]);
			close(p[0]);
			close(p[1]);
			run_single_cmd(i, args);
		}
		else
		{
			close(0);
			dup(p[0]);
			close(p[0]);
			close(p[1]);
			run_pipe_cmd(argc - i - 1, args + i + 1);
		}
	}
	else
	{
		run_single_cmd(argc, args);
	}
}
/*
* 对于single_cmd, 将其看作一个command与params以及其它重定向语句组合而成, 即single_cmd = command + params + redir
* 那些重定向语句只对当前进程有效, 并且重定向语句要先处理, 之后再调用exec执行command与params
*/
void run_single_cmd(int argc, char **args)
{
	while (try_to_redir(argc, args) > 0)
	{
		;
	}
	int i = 0;
	for (i = 0; i < argc; i++)
	{
		if (args[i] != 0)
		{
			break;
		}
	}
	

	// fprintf(2,"start exec %s\n",args[i]);
	// for(int j = 0; *(args + i + j); j++){
	// 	fprintf(2,".%s",*(args+i+j));
	// }
	exec(args[i], args + i);
}
/*
* 找到重定向语句并执行, 先找'>','<','>>'三个标识, 找到后再取得跟在这些符号后面的文件名
* 需要说明的是, 在本实验中'>>'与'>'效果相同(sh.c中也将'>>'与'>'处理的语句相同)
*/
int try_to_redir(int argc, char **args)
{
	int i = 0;
	int token = 0;
	for (i = 0; i < argc; i++)
	{
		if (strcmp(args[i], "<") == 0)
		{
			token = '<';
			break;
		}
		if (strcmp(args[i], ">") == 0)
		{
			token = '>';
			break;
		}
		if (strcmp(args[i], ">>") == 0)
		{
			token = '+';
			break;
		}
	}
	if (token != 0 || i != argc)
	{
		switch (token)
		{
		case '<':
			close(0);
			//fprintf(2,"change 0 to file %s\n",args[i + 1]);
			if (open(args[i + 1], O_RDONLY) < 0)
			{
				fprintf(2, "open %s failed\n", args[i + 1]);
				exit(-1);
			}
			break;
		case '>':
			close(1);
			//fprintf(2,"change 1 to file %s\n",args[i + 1]);
			if (open(args[i + 1], O_WRONLY | O_CREATE) < 0)
			{
				fprintf(2, "open %s failed\n", args[i + 1]);
				exit(-1);
			}
			break;
		case '+':
			close(1);
			//fprintf(2,"change 1 to file %s\n",args[i + 1]);
			if (open(args[i + 1], O_WRONLY | O_CREATE) < 0)
			{
				fprintf(2, "open %s failed\n", args[i + 1]);
				exit(-1);
			}
			break;
		}
		args[i] = 0;
		args[i + 1] = 0;
		return 1;
	}
	return -1;
}

int main(void)
{
	static char buf[100];
	int fd;

	// Ensure that three file descriptors are open.
	
	while ((fd = open("console", O_RDWR)) >= 0)
	{
		if (fd >= 3)
		{
			close(fd);
			break;
		}
	}
	// Read and run input commands.
	while (getcmd(buf, sizeof(buf)) >= 0)
	{
		
		//fprintf(2, "xxx:---%s\n", buf);
		if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
		{
			// nsh不需要支持cd指令
			fprintf(2, "nsh doesn't support cd\n");
		}
		int str_len = strlen(buf);
		buf[str_len - 1] = 0;
		if (fork1() == 0)
		{
			char *args[MAXARGS];
			int arg_num = 0;
			char *p = buf;
			// 获取输入, 根据空格将其中的参数字符串切分出来
			while (*p)
			{
				while (*p && *p == ' ')
				{
					*p = 0;
					p++;
				}
				if (*p)
				{
					args[arg_num++] = p;
				}
				while (*p && *p != ' ')
				{
					if(*p == '\n'){
						*p = 0;
					}
					p++;
				}
			}
			args[arg_num] = 0;
			//fprintf(2,"only exec once, %d\n",arg_num);
			run_pipe_cmd(arg_num, args);
		}
		wait(0);
	}
	exit(0);
}

void panic(char *s)
{
	fprintf(2, "%s\n", s);
	exit(-1);
}

int fork1(void)
{
	int pid;

	pid = fork();
	if (pid == -1)
		panic("fork");
	return pid;
}
