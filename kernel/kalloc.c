// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint* ref_count;  // ref_count代表每个page的ref数, 设置此ref是为了fork时父子进程的vma能同时看见同一物理内存的文件
                    // ref_count在trap中给mem加1,在fork中加1,在exit中减1, 在munmap的时候减1
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  uint total_page_numbers =((PHYSTOP - (uint64)end) >> 12) + 1; // 总的页数
  uint ref_count_offset = ((total_page_numbers * (sizeof(uint)) >> 12) + 1) << 12;  // 这里计算出为每页存一个ref_count需要的页的字节数
  kmem.ref_count = (uint*)end;
  freerange(end + ref_count_offset, (void*)PHYSTOP);
}
// 使物理地址pa处的page的rec_count加1, pa必须是page的开始地址
void
kref(void *pa){
  uint64 idx = ((uint64)pa - (uint64)end) >> 12;

  acquire(&kmem.lock);
  kmem.ref_count[idx]++;
  release(&kmem.lock);
}
// 使物理地址pa处的page的rec_count减1, 若减为0则free掉该page
// pa必须是page的开始地址
void
kderef(void *pa){
  uint64 idx = ((uint64)pa - (uint64)end) >> 12;
  int should_free = 0;
  acquire(&kmem.lock);
  kmem.ref_count[idx]--;
  if(kmem.ref_count[idx] == 0){
    should_free = 1;  // 必须要先释放kmem.lock再掉用kfree,否则死锁
  }
  release(&kmem.lock);

  if(should_free){
    kfree(pa);
  }
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
