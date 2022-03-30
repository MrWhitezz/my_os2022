#include <common.h>
#include <pmm.h>

unsigned int nextPower_2(unsigned int x){
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  return x + 1;
}


static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}

#ifndef TEST
static void pmm_init() {
  // memset(lk, SPIN_INIT(), sizeof(lk)); // abuse of SPIN_INIT() == 0
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

}
#else
// 测试代码的 pmm_init ()

#define HEAP_SIZE (128 * (1 << 20))
extern  Area   heap;
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
