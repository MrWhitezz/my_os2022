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

static int get_slab_index(size_t x){
  int index = 0;
  while (x > 16) {
    x = x >> 1;
    index++;
  }
  return index;
}

void do_test_func(){
  // for (int i = 0; i < 0x1145; ++i){
  //   printf("%d index: %d\n", i, get_slab_index(i));
  // }
  printf("%d index: %d\n", 32, get_slab_index(32));
  printf("%d index: %d\n", 33, get_slab_index(33));
  size_t tmp = 33;
  while (tmp > 16)
  {
    printf("%d\n", tmp);
    tmp = tmp >> 1;
  }
  

}

void do_test_1(){
  #define ALLOC_SZ 14
  uint32_t size = 0;
  for (int i = 0; i < 15000; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < ALLOC_SZ; ++j){
      // Log("try alloc\n");
      ptr[j] = pmm->alloc(1 << j);
      // Log("alloc success, try to free\n");
      // pmm->free(ptr[j]);
      // printf("alloc 0x%x at %p\n", 1 << j, ptr[j]);
      size += 1 << j;
    }
    if (i % 1000 == 0) 
      printf("ptr[0] = %p\n", ptr[0]);
  }
  printf("Total size: %d MiB\n", size >> 20);
}

int main() {
  pmm->init();
  do_test_func();
  for (int i = 0; i < 1; i++)
    create(do_test_1);
  join(goodbye);
  Log("End of main.\n");
}