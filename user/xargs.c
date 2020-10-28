#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char *buf[MAXARG];
    for (int i = 0; i < MAXARG; i++)
    {
        buf[i] = (char *)malloc(512);
    }
    char *command = argv[1];

    int line_count = 1;
    while (read(0, buf[line_count], 512) > 0)
    {
        int str_len = strlen(buf[line_count]);
        if (str_len < 1)
            break;
        buf[line_count][str_len - 1] = 0;
        line_count++;
    }

    char *n_params[MAXARG];
    for (int i = 1; i < argc; i++)
    {
        n_params[i - 1] = argv[i];
    }
    
    for (int i = 1; i < line_count; i++)
    {
        int arg_num = argc - 1;
        char *p = buf[i];
        while (*p)
        {
            while (*p && *p == ' ')
            {
                *p = 0;
                p++;
            }
            if (*p)
            {
                n_params[arg_num++] = p;
            }
            while (*p && *p != ' ')
            {
                if(*p == '\n'){
                    *p = 0;
                    p++;
                    break;
                }
                p++;
            }
        }
        if(arg_num > MAXARG){
            printf("Too many args in line %d", line_count);
            continue;
        }
        n_params[arg_num] = 0;
        if (fork() == 0)
        {
            exec(command, n_params);
            exit();
        }
        wait();
    }

    exit();
}
