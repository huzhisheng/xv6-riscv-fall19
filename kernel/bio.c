// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13
#define HASHKEY 590127
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head[NBUCKET];
  struct spinlock bucketlock[NBUCKET];
} bcache;

int bhash(int blockno){
  return (blockno + HASHKEY)%NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(int i=0;i<NBUCKET;i++){
    bcache.head[i].prev = &(bcache.head[i]);
    bcache.head[i].next = &(bcache.head[i]);
    initlock(&(bcache.bucketlock[i]), "bcache.bucketlock");
  }
  
  int blockno = 0;

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int bucketno = bhash(blockno);  //将所有buf均匀分给每个bucket
    b->next = bcache.head[bucketno].next;
    b->prev = &(bcache.head[bucketno]);
    initsleeplock(&b->lock, "buffer");
    bcache.head[bucketno].next->prev = b;
    bcache.head[bucketno].next = b;
    blockno++;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  
  struct buf *b;
  int bucketno = bhash(blockno);
  //printf("bucketno: %d",bucketno);
  //acquire(&bcache.lock);
  acquire(&(bcache.bucketlock[bucketno]));
  // Is the block already cached?
  for(b = bcache.head[bucketno].next; b != &bcache.head[bucketno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      //release(&bcache.lock);
      release(&(bcache.bucketlock[bucketno]));
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  for(b = bcache.head[bucketno].prev; b != &bcache.head[bucketno]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      //release(&bcache.lock);
      release(&(bcache.bucketlock[bucketno]));
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  //如果没找到空的buf,就取出一个head的尾部buf拿来用了
  b = bcache.head[bucketno].prev;
  acquiresleep(&b->lock);
  bwrite(b);
  releasesleep(&b->lock);
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bcache.head[bucketno].next;
  b->prev = &(bcache.head[bucketno]);
  bcache.head[bucketno].next->prev = b;
  bcache.head[bucketno].next = b;
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  release(&(bcache.bucketlock[bucketno]));
  acquiresleep(&b->lock);
  return b;
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucketno = bhash(b->blockno);
  acquire(&(bcache.bucketlock[bucketno]));
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bucketno].next;
    b->prev = &(bcache.head[bucketno]);
    bcache.head[bucketno].next->prev = b;
    bcache.head[bucketno].next = b;
  }
  
  release(&(bcache.bucketlock[bucketno]));
}

void
bpin(struct buf *b) {
  int bucketno = bhash(b->blockno);
  //acquire(&bcache.lock);
  acquire(&(bcache.bucketlock[bucketno]));
  b->refcnt++;
  //release(&bcache.lock);
  release(&(bcache.bucketlock[bucketno]));
}

void
bunpin(struct buf *b) {
  int bucketno = bhash(b->blockno);
  //acquire(&bcache.lock);
  acquire(&(bcache.bucketlock[bucketno]));
  b->refcnt--;
  //release(&bcache.lock);
  release(&(bcache.bucketlock[bucketno]));
}


