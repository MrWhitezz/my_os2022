#include <common.h>


static void os_init() {
  // single processor
  pmm->init();
  // should segfault
  kmt->init();
}


static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  iset(true);
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
