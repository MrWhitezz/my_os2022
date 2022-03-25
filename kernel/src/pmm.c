#include <common.h>
#include <pmm.h>

extern spinlock_t lk[LK_SIZE];
extern __node_t * head;
extern uint32_t list_size;

static void *kalloc(size_t size) {
  if (size <= sizeof(header_t))
    size = sizeof(header_t); 
  else if (size >= MAX_ALLOC)
    return NULL;
  
  return list_alloc(size);
}

static void kfree(void *ptr) {
  list_free(ptr);
}

#ifndef TEST
static void pmm_init() {
  list_init();
  memset(lk, SPIN_INIT(), sizeof(lk)); // abuse of SPIN_INIT() == 0
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

}
#else
// 测试代码的 pmm_init ()

#define HEAP_SIZE (128 * (1 << 20))
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);

  list_init();
  memset(lk, SPIN_INIT(), sizeof(lk)); // abuse of SPIN_INIT() == 1
}
#endif

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
