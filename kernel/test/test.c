#include <common.h>
#include <thread.h>
#include <stdatomic.h>

static void entry(int tid) { pmm->alloc(128); }
static void goodbye()      { printf("End.\n"); }
int atomic_xchg(int *addr, int newval) {
  return atomic_exchange((int *)addr, newval);
}
int      cpu_count   (void) {return 8;}
int      cpu_current (void) {return 0;}
Area heap = {};

void do_test_1(){
  #define ALLOC_SZ 14
  uint32_t size = 0;
  for (int i = 0; i < 10000; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < (1 << ALLOC_SZ); j += 114){
      // Log("try alloc\n");
      ptr[j] = pmm->alloc(j);
      // Log("alloc success, try to free\n");
      // pmm->free(ptr[j]);
      // printf("alloc 0x%x at %p\n", 1 << j, ptr[j]);
      if (ptr[j] != NULL)
          size += j;
    }
    if (i % 1000 == 0) 
      printf("ptr[0] = %p\n", ptr[0]);
  }
  printf("Total size: %d MiB\n", size >> 20);
}

int main() {
  pmm->init();
  for (int i = 0; i < 1; i++)
    create(do_test_1);
  join(goodbye);
  Log("End of main.\n");
}