#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
//本实验的xv6的xargs与linux中真正的xargs略有不同,
//对于本xargs的非第一行的参数,要将其附加在第一行来执行,
//一开始我以为只是用第一行的命令去执行之后几行的参数即可
//比如echo 12 
//34
//56
//真正的xargs应该是输出12 34 56 但从老师处询问才得知本实验的要求是将第一行附加在其余行前面执行, 则结果就变成了12 34 (换行) 12 56
int main(int argc, char *argv[])
{
    char *buf[MAXARG];
    for (int i = 0; i < MAXARG; i++)
    {
        buf[i] = (char *)malloc(512);
    }
    char *command = argv[1];

    //先将输入的每一行读取到buf中
    int line_count = 1;
    while (read(0, buf[line_count], 512) > 0)
    {
        int str_len = strlen(buf[line_count]);
        if (str_len < 1)
            break;
        buf[line_count][str_len - 1] = 0;
        line_count++;
    }
    // n_params是(从第二行开始)每一行与第一行进行拼接成的参数列表
    char *n_params[MAXARG];
    // 先将第一行的参数复制到n_params
    for (int i = 1; i < argc; i++)
    {
        n_params[i - 1] = argv[i];
    }
    // 从第二行开始, 读取参数附加到第一行n_params的后面
    for (int i = 1; i < line_count; i++)
    {
        int arg_num = argc - 1;
        char *p = buf[i];
        while (*p)
        {
            while (*p && *p == ' ')     //跳过空格, 并将这些空格变成'\0'
            {
                *p = 0;
                p++;
            }
            if (*p)
            {
                n_params[arg_num++] = p;
            }
            while (*p && *p != ' ')     //跳过非空格内容, 检测到换行符则break
            {
                if(*p == '\n'){
                    *p = 0;
                    p++;
                    break;
                }
                p++;
            }
        }
        if(arg_num > MAXARG){   //参数数量超过限制
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
