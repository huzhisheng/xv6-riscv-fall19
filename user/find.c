#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *file_name)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0)
	{
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}
	if (fstat(fd, &st) < 0)
	{
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}
	while (read(fd, &de, sizeof(de)) == sizeof(de))
	{
		if (de.inum == 0)
			continue;
		strcpy(buf,path);
		p = buf + strlen(buf);
		*p++ = '/';
		
		memmove(p,de.name,DIRSIZ);
		p[DIRSIZ] = 0;
		if(stat(buf,&st) < 0){
			printf("find: cannot stat %s",buf);
			continue;
		}
		
		switch (st.type)
		{
		case T_FILE:
			if (strcmp(de.name, file_name) == 0)
			{
				printf("%s\n", buf);
			}
			break;

		case T_DIR:
			if (strlen(buf) + 1 + DIRSIZ + 1 > sizeof buf)
			{
				printf("ls: path too long\n");
				break;
			}
			if(strcmp(de.name,".")!=0 && strcmp(de.name,"..")!=0)
				find(buf,file_name);
			break;
		}
	}

	close(fd);
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		write(2, "Error in args", strlen("Error in args"));
		exit();
	}
	find(argv[1], argv[2]);
	exit();
}
