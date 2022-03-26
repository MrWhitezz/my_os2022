#include <common.h>
#include <thread.h>
#include <stdatomic.h>

static void entry(int tid) { pmm->alloc(128); }
static void goodbye()      { printf("End.\n"); }
int atomic_xchg(int *addr, int newval) {
  return atomic_exchange((int *)addr, newval);
}
Area heap = {};

void do_test_1(){
  #define ALLOC_SZ 12
  for (int i = 0; i < 5; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < ALLOC_SZ; ++j){
      ptr[j] = pmm->alloc(1 << j);
      printf("alloc 0x%x at %p\n", 1 << j, ptr[j]);
    }
  }
}

int main() {
  pmm->init();
  // for (int i = 0; i < 1; i++)
  //   create(do_test_1);
  join(goodbye);
}