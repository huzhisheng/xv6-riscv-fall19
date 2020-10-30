#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// 参考sh.c

#define MAXARGS 50

int fork1(void); // Fork but panics on failure.
void panic(char *);
void run_single_cmd(int argc, char **args);
int try_to_redir(int argc, char **args);

int getcmd(char *buf, int nbuf)
{
	fprintf(2, "@ ");
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
		return -1;
	return 0;
}

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
		
		if (fork1() == 0)
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
