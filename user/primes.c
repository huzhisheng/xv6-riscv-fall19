#include "kernel/types.h"
#include "user/user.h"

void redirect(int k, int pp[]){
    close(k);
    dup(pp[k]);
    close(pp[0]);
    close(pp[1]);
}



void filtering_data(int prime){
    int n;
    while(read(0,&n,sizeof(n))){
        if(n % prime != 0){
            write(1,&n,sizeof(n));
        }
    }
}

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
        for(int i = 2;i < 36;i++){
            write(1,&i,sizeof(i));
        }
    }
    exit();
}