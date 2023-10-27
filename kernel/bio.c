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

#define NBUCKETS 13

// 使用哈希锁

struct
{
  struct spinlock globalLock;
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
  struct buf hashbucket[NBUCKETS]; // 每个哈希队列一个linked list及一个lock
} bcache;

void binit(void)
{
  struct buf *b;
  // 初始化全局分配大锁
  initlock(&bcache.globalLock, "bcache");
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "bcache.bucket");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint hashNumber = blockno % NBUCKETS;
  acquire(&bcache.lock[hashNumber]);

  // Is the block already cached?
  for (b = bcache.hashbucket[hashNumber].next; b != &bcache.hashbucket[hashNumber]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[hashNumber]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 先看自己的桶里有没有空闲块
  for (b = bcache.hashbucket[hashNumber].prev; b != &bcache.hashbucket[hashNumber]; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[hashNumber]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[hashNumber]);

  // 如果自己的桶里没有空闲块：
  // 获取全局分配大锁
  acquire(&bcache.globalLock);
  // 获取原有桶的锁
  acquire(&bcache.lock[hashNumber]);
  for (int i = 0; i < NBUCKETS; i++)
  {
    if (i == hashNumber)
      continue;
    // 获取新桶的锁
    acquire(&bcache.lock[i]);
    for (b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev)
    {
      if (b->refcnt == 0)
      // 在其他哈希桶中找到了一个空闲块
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // 将b块移动到hashNumber对应的块中
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.hashbucket[hashNumber].next;
        b->prev = &bcache.hashbucket[hashNumber];
        bcache.hashbucket[hashNumber].next->prev = b;
        bcache.hashbucket[hashNumber].next = b;
        // 释放新桶的锁
        release(&bcache.lock[i]);
        acquiresleep(&b->lock);
        // 释放原有桶的锁
        release(&bcache.lock[hashNumber]);
        // 释放全局分配大锁
        release(&bcache.globalLock);
        return b;
      }
    }
    // 如果一个桶没找到，释放该桶的锁，继续进入for循环
    release(&bcache.lock[i]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  uint hashNumber = b->blockno % NBUCKETS;
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock[hashNumber]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[hashNumber].next;
    b->prev = &bcache.hashbucket[hashNumber];
    bcache.hashbucket[hashNumber].next->prev = b;
    bcache.hashbucket[hashNumber].next = b;
  }

  release(&bcache.lock[hashNumber]);
}

void bpin(struct buf *b)
{
  uint hashNumber = b->blockno % NBUCKETS;
  acquire(&bcache.lock[hashNumber]);
  b->refcnt++;
  release(&bcache.lock[hashNumber]);
}

void bunpin(struct buf *b)
{
  uint hashNumber = b->blockno % NBUCKETS;
  acquire(&bcache.lock[hashNumber]);
  b->refcnt--;
  release(&bcache.lock[hashNumber]);
}
