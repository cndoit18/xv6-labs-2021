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
#define HASHBUCKET(x) ((x)%NBUCKET) 

struct bucket
{
  struct spinlock lock;

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
};

struct {
  struct buf buf[NBUF];
  
  struct spinlock lock;
  struct bucket buckets[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  struct bucket *bt;
  int i = 0;

  initlock(&bcache.lock, "bcache");
  for(bt = bcache.buckets; bt < bcache.buckets + NBUCKET; bt ++) {
    initlock(&bt->lock, "bucket");
    bt->head.prev = &bt->head;
    bt->head.next = &bt->head;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    bt = bcache.buckets + HASHBUCKET(i);
    
    b->next = bt->head.next;
    b->prev = &bt->head;
    initsleeplock(&b->lock, "buffer");
    bt->head.next->prev = b;
    bt->head.next = b;

    i = HASHBUCKET(i+1);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct bucket *bt, *t;

  bt = bcache.buckets + HASHBUCKET(blockno);
  acquire(&bt->lock);
  // Is the block already cached?
  for(b = bt->head.next; b != &bt->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bt->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(uint i = HASHBUCKET(blockno + 1); i != HASHBUCKET(blockno); i = HASHBUCKET(i + 1)) {
    t = bcache.buckets + i;
    acquire(&t->lock);

    for(b = t->head.prev; b != &t->head; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&t->lock);

        b->next = bt->head.next;
        b->prev = &bt->head;
        bt->head.next->prev = b;
        bt->head.next = b;
        release(&bt->lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&t->lock);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  struct bucket *bt;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  bt = bcache.buckets + HASHBUCKET(b->blockno);
  acquire(&bt->lock);
  
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bt->head.next;
    b->prev = &bt->head;
    bt->head.next->prev = b;
    bt->head.next = b;
  }
  
  release(&bt->lock);
}

void
bpin(struct buf *b) {
  struct bucket *bt = bcache.buckets + HASHBUCKET(b->blockno);
  acquire(&bt->lock);
  b->refcnt++;
  release(&bt->lock);
}

void
bunpin(struct buf *b) {
  struct bucket *bt = bcache.buckets + HASHBUCKET(b->blockno);
  acquire(&bt->lock);
  b->refcnt--;
  release(&bt->lock);
}


