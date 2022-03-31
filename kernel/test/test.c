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
  Log("begin test 1\n");
  srand(time(NULL));
  #define ALLOC_SZ 14
  uint32_t size = 0;
  for (int i = 0; i < 10000; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < ALLOC_SZ; j ++){
      // Log("try alloc\n");
      size_t sz = (size_t)(rand() % 100);
      ptr[j] = pmm->alloc(j);
      printf("alloc %ld at %p\n", sz, ptr[j]);
      if (!(ROUNDUP((uintptr_t)ptr[j], nextPower_2(sz)) == (uintptr_t)ptr[j])){
        printf("0x%x 0x%x\n", ROUNDUP((uintptr_t)ptr[j], nextPower_2(sz)), (uintptr_t)ptr[j]);
      }
      assert(ROUNDUP((uintptr_t)ptr[j], nextPower_2(sz)) == (uintptr_t)ptr[j]);
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
  Log("end test 1\n");
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
      printf("alloc 0x%ld at %p\n", sz, ptr);
      assert(ROUNDUP((uintptr_t)ptr, sz) == (uintptr_t)ptr);
      pmm->free(ptr);
    }
  }
}

int main(int argc, char *argv[]) {
  pmm->init();
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 1: do_test_1();
    case 2: do_test_2();
    case 3: do_test_3();
  }
  Log("End of main.\n");
}