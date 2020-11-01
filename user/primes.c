#include "kernel/types.h"
#include "user/user.h"

/*
重定位函数,将k文件描述符先关闭(使其空闲出来),再通过dup函数将pipe的某一端占用到k上
*/
void redirect(int k, int pp[]){ 
    close(k);
    dup(pp[k]);
    close(pp[0]);   //这里将pipe原来的两个端口关闭,避免因为未关闭多余端口而导致pipe read端一直等待
    close(pp[1]);
}


//将prime倍数的数给筛选掉
void filtering_data(int prime){
    int n;
    while(read(0,&n,sizeof(n))){
        if(n % prime != 0){
            write(1,&n,sizeof(n));
        }
    }
}
//父进程带着最前面传进来的数进入下一层, 子进程留下来将其余的数筛选后传入下一层
void go_next_layer(){
    int pp[2];
    int prime;
    if(read(0,&prime,sizeof(prime))){
        printf("prime %d\n",prime);
        pipe(pp);
        if(fork()){
            redirect(0,pp);
            go_next_layer();
        }else{
            redirect(1,pp);
            filtering_data(prime);
        }
    }
}

int main(){
    int pp[2];
    pipe(pp);
    if(fork()){
        redirect(0,pp);
        go_next_layer();
    }else{
        redirect(1,pp);
        for(int i = 2;i < 36;i++){  //数字从2到35
            write(1,&i,sizeof(i));
        }
    }
    exit();
}