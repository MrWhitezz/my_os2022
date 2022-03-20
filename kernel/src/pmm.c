#include <common.h>

// Spinlock
typedef int spinlock_t;
#define SPIN_INIT() 0

// int atomic_xchg(int *addr, int newval) {
//   int result;
//   asm volatile ("lock xchg %0, %1":
//     "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
//   return result;
// }

void spin_lock(spinlock_t *lk) { while (atomic_xchg(lk, 1))  ; }
void spin_unlock(spinlock_t *lk) { atomic_xchg(lk, 0); }

typedef struct used_node {
  Area area;
  struct used_node* next;
} used_node;

used_node* H;

static void *kalloc(size_t size) {
  
  return malloc(size);
}

static void kfree(void *ptr) {
  free(ptr);
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
