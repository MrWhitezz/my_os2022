#include <common.h>


static void os_init() {
  pmm->init();
}

void do_test_1(){
  #define ALLOC_SZ 12
  uint32_t size = 0;
  for (int i = 0; i < 15000; i++) {
    void *ptr[ALLOC_SZ];
    for (int j = 0; j < ALLOC_SZ; ++j){
      ptr[j] = pmm->alloc(1 << j);
      // printf("alloc 0x%x at %p\n", 1 << j, ptr[j]);
      size += 1 << j;
    }
    if (i % 1000 == 0) 
      printf("ptr[0] = %p\n", ptr[0]);
  }
  printf("Total size: %d MiB\n", size >> 20);
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  // do_test_1();
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
