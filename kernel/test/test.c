#include <common.h>
#include <thread.h>
#include <stdatomic.h>

static void entry(int tid) { pmm->alloc(128); }
static void goodbye()      { printf("End.\n"); }
int atomic_xchg(int *addr, int newval) {
  return atomic_exchange((int *)addr, newval);
}
int      cpu_count   (void) {return 1;}
int      cpu_current (void) {return 0;}
Area heap = {};

void do_test_1(){
  Log("begin test 1\n");
  srand(time(NULL));
  #define ALLOC_SZ 100
  uint32_t size = 0;
  for (int i = 0; i < 100000; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < ALLOC_SZ; j ++){
      // Log("try alloc\n");
      size_t sz = (size_t)(rand() % 10000) + 1;
      ptr[j] = pmm->alloc(sz);
      // printf("alloc %ld at %p\n", sz, ptr[j]);
      if (!(ROUNDUP((uintptr_t)ptr[j], nextPower_2(sz)) == (uintptr_t)ptr[j])){
        size_t al_sz = nextPower_2(sz);
        assert(ROUNDUP((uintptr_t)ptr[j], nextPower_2(sz)) == (uintptr_t)ptr[j]);
      }
      if (ptr[j] != NULL)
        pmm->free(ptr[j]);
      // Log("alloc success, try to free\n");
      // pmm->free(ptr[j]);
      // printf("alloc 0x%x at %p\n", 1 << j, ptr[j]);
      if (ptr[j] != NULL)
          size += j;
    }
    // if (i % 100000 == 0) printf("ptr[0] = %p\n", ptr[0]);
  }
  printf("Total size: %d K\n", size >> 10);
  printf("Total size: %d MiB\n", size >> 20);
  Log("end test 1\n");
}

void do_test_2(){
  // trivial test; to be modified
  Log("begin test 2\n");
  #define PT_SZ 100
  size_t total_size = 0;
  void *ptr[PT_SZ];
  for (int i = 0; i < 1000; ++i){
    for (int j = 0; j < PT_SZ; ++j){
      size_t sz = (size_t)(rand() % 1000) + 1;
      ptr[j] = pmm->alloc(1 << j);
      if (ptr[j] != NULL){
        total_size += sz;
        // printf("alloc %ld at %p\n", sz, ptr[j]);
      }
    }
    for (int j = 0; j < PT_SZ; ++j){
      if (ptr[j] != NULL)
        pmm->free(ptr[j]);
    }
  }
  printf("Total size: %ld MiB\n", total_size >> 20);

  Log("end test 2\n");
}

void do_test_3(){
  Log("begin test 3\n"); 
  srand(time(NULL));
  size_t tot_sz = 0;
  const int arr_sz = 8;
  void *ptr[arr_sz];
  for (int j = 0; j < 100000; ++ j){
    for (int i = 0; i < arr_sz; ++i){
      size_t rd = rand() % 1000;
      size_t sz = 64 * 1024 * (rd + 1);
      ptr[i] = pmm->alloc(sz);
      // printf("alloc 0x%ld at %p\n", sz, ptr);
      assert(ROUNDUP((uintptr_t)ptr[i], nextPower_2(sz)) == (uintptr_t)ptr[i]);
      if (ptr[i] != NULL)
        tot_sz += sz;
    }
    for (int i = 0; i < arr_sz; ++i){
      if (ptr[i] != NULL)
        pmm->free(ptr[i]);
    }
  }
  printf("Total size: %ld MiB\n", tot_sz >> 20);
  Log("end test 3\n");
}

int main(int argc, char *argv[]) {
  pmm->init();
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 1: do_test_1(); break;
    case 2: do_test_2(); break;
    case 3: do_test_3(); break;
  }
  Log("End of main.\n");
}