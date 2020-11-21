//与其说这是file的结构体,不如说这是file decripter结构体
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE and FD_DEVICE
  short major;       // FD_DEVICE
  short minor;       // FD_DEVICE
};
// major number：表示不同的设备类型
// minor number：表示同一个设备的的不同分区
#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number 就是inode在磁盘中连续inode区域里的下标
  int ref;            // Reference count 指向该in-mem inode的C指针数量, 当这个为0时inode就被放回到mem中
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;          // 和dinode中的size相同, 文件的字节大小数目
  uint addrs[NDIRECT+2];  // addr是内容block的blocknum
};

// map major device number to device functions.
struct devsw {
  int (*read)(struct file *, int, uint64, int);
  int (*write)(struct file *, int, uint64, int);
};

extern struct devsw devsw[];

#define DISK 0
#define CONSOLE 1
