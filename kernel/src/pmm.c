#include <common.h>
#include <pmm.h>


// typedef struct used_node {
//   Area area;
//   struct used_node* next;
// } used_node;

// used_node* H;
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
  // always give a larger size
  size = nextPower_2(size);

  while(1){
    // TODO
    assert(curr != NULL);
    // something wrong
    header_t * h = (header_t*)curr;
    assert(h != NULL);
  }
  // release lock
  spin_unlock(&lk1);

}

static void *kalloc(size_t size) {
  
  return malloc(size);
}

static void kfree(void *ptr) {
  // free(ptr);
}

static void pmm_init() {
  list_init();
  lk1 = SPIN_INIT();
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
