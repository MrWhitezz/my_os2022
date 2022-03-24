#include <pmm.h>
#include <common.h>

// Spin lock
void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

unsigned int nextPower_2(unsigned int x){
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}

spinlock_t lk1;
__node_t * head;
uint32_t list_size;


void list_init(){
  list_size = ((uintptr_t)heap.end - (uintptr_t)heap.start) / 2;
  head = (__node_t*)heap.start;
  head->size = list_size;
  head->next = NULL;
}

void* list_alloc(size_t size){
  assert(size >= sizeof(struct header_t));
  // add lock
  spin_lock(&lk1);
  __node_t * curr = head;
  void * ret = NULL;
  // always give a larger size
  size = nextPower_2(size);

  while(1){
    // TODO
    if (curr == NULL){
      break;
    }
    uint32_t free_sz = curr->size - ((ROUNDUP(curr, size) - (uintptr_t)curr)); 
    if (free_sz >= size){
      ret = (void *)ROUNDUP(curr, size);
      // TODO: move the curr to ret + size
      break;
    }
    curr = curr->next;
    // something wrong
    header_t * h = (header_t*)curr;
    assert(h != NULL);
  }
  // release lock
  spin_unlock(&lk1);
  return ret;

}
