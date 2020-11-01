#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAXLINE 100
int main(int argc, char *argv[]) {
    char buf[MAXLINE];
    int parent_fd[2];
    int child_fd[2];
    int ret_parent;
    int ret_child;
    ; 
    if((ret_parent = pipe(parent_fd)) < 0 || (ret_child = pipe(child_fd)) < 0){
        write(2, "Crate pipe failed", strlen("Crate pipe failed"));
    }
    
    if(fork() == 0){    //子进程
        read(parent_fd[0],buf,MAXLINE);
        printf("%d: received %s\n",getpid(),buf);
        close(child_fd[0]);     //子进程读完后,关闭pipe读取端
        write(child_fd[1],"pong",5);
    }else{          //父进程
        close(parent_fd[0]);    //父进程不需要从pipe中读,因此直接关闭读取端
        write(parent_fd[1],"ping",5);
        read(child_fd[0],buf,MAXLINE);
        printf("%d: received %s\n",getpid(),buf);
    }
    exit();
}