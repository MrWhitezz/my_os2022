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
      pmm->free(ptr[j]);
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

void do_test_2(){
  for (int i = 0; i < 10; ++i){
    void *ptr = pmm->alloc(128 * 1024);
    printf("alloc 0x%x at %p\n", 128, ptr);
    pmm->free(ptr);
  }
}

void do_test_3(){
  for (int j = 0; j < 10; ++ j){
    for (int i = 0; i < 4; ++i){
      size_t sz = 64 * 1024 * (1 << i);
      void *ptr = pmm->alloc(sz);
      printf("alloc 0x%x at %p\n", sz, ptr);
      assert(ROUNDUP((uintptr_t)ptr, sz) == (uintptr_t)ptr);
      pmm->free(ptr);
    }
  }
}

int main() {
  pmm->init();
  // for (int i = 0; i < 1; i++)
  //   create(do_test_1);
  do_test_2();
  do_test_3();
  // join(goodbye);
  Log("End of main.\n");
}