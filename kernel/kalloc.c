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
  uint *ref_count;  //在物理内存的前面一部分存储页的引用数量
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.ref_count = (uint*)end;
  uint64 page_counts = ((PHYSTOP - (uint64)end) >> 12)+1;  //加1是为了最后一页的ref_count冗余留出来
  uint64 pa_offset = page_counts * sizeof(uint);
  memset(end,0,pa_offset);
  //printf("kinit %d %d\n",pa_offset,page_counts);
  freerange(end + pa_offset, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    k_real_free(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
k_real_free(void *pa)
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
void kfree(void *pa){
  uint64 index = ((uint64)pa-(uint64)end)>>12;
  uint ref_count;
  acquire(&kmem.lock);
  kmem.ref_count[index]--;
  ref_count = kmem.ref_count[index];
  //printf("ref_count: %d\n", ref_count);
  release(&kmem.lock);

  if(ref_count == 0)
    k_real_free(pa);
}
void klink(void *pa){
  //printf("test");
  uint64 index = ((uint64)pa-(uint64)end)>>12;
  //printf("test %p %p %d",(uint64)pa,(uint64)end,index);
  acquire(&kmem.lock);
  kmem.ref_count[index]++;
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

  klink(r);
  return (void*)r;
}
