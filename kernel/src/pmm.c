#include <common.h>
#include <pmm.h>


// typedef struct used_node {
//   Area area;
//   struct used_node* next;
// } used_node;

// used_node* H;
extern spinlock_t lk1;
extern __node_t * head;
extern uint32_t list_size;

static void *kalloc(size_t size) {
  if (size <= sizeof(header_t))
    size = sizeof(header_t); 
  return list_alloc(size);
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
