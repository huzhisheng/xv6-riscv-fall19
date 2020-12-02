//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  printf("sockrecvudp\n");
  struct sock* pos = sockets;
  acquire(&lock);
  while(pos){
    if (pos->raddr == raddr && pos->lport == lport && pos->rport == rport){
      release(&lock);
      break;
    }else{
      pos = pos->next;
    }
  }
  if(pos){
    mbufq_pushtail(&pos->rxq, m);
    wakeup(&pos->rxq);
  }else{
    release(&lock);
    mbuffree(m);
  }
}

void
sockclose(struct sock* si){
  printf("sockclose\n");
  struct sock* pos;
  pos = sockets;
  acquire(&si->lock);

  if(pos == si){
    sockets = si->next;
  }else{
    acquire(&lock);
    while (pos && pos->next != si) {
      pos = pos->next;
    }
    if(!pos || pos->next != si){
      release(&lock);
      release(&si->lock);
      return;
    }
    pos->next = si->next;
    release(&lock);
  }
  while(!mbufq_empty(&si->rxq)){
    struct mbuf* temp_buf = mbufq_pophead(&si->rxq);
    mbuffree(temp_buf);
  }
  release(&si->lock);
  kfree((char*)si);
  return;
  
}

int
sockread(struct sock* si, uint64 addr, int n){
  printf("sockread\n");
  struct proc *pr = myproc();
  acquire(&(si->lock));
  while(mbufq_empty(&(si->rxq))){
    if(myproc()->killed){
      release(&(si->lock));
      return -1;
    }
    sleep(&si->rxq, &(si->lock)); //DOC: piperead-sleep
  }
  struct mbuf* buffer = mbufq_pophead(&(si->rxq));
  if(n > buffer->len)   // 第一次显示didn't receive correct payload是由于sockread返回的已读取字节数错误，修改后能通过one ping和single_process
    n = buffer->len;
  if(copyout(pr->pagetable,addr,buffer->head,n) == -1){
    release(&si->lock);
    mbuffree(buffer);
    return -1;
  }
  release(&si->lock);
  mbuffree(buffer);
  return n;
}

int
sockwrite(struct sock* si, uint64 addr, int n){
  printf("sockwrite\n");
  struct proc *pr = myproc();
  //acquire(&si->lock);
  struct mbuf* buffer = mbufalloc(sizeof(struct udp) + sizeof(struct ip) + sizeof(struct eth));
  mbufput(buffer, n);
  if(copyin(pr->pagetable,buffer->head,addr,n)==-1){
    mbuffree(buffer);
    return -1;
  }
  net_tx_udp(buffer,si->raddr,si->lport,si->rport);

  //release(&si->lock);
  return n;
}