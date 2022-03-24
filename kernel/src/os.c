#include <common.h>


static void os_init() {
  pmm->init();
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  for (unsigned x = 0; x < 0x11; x++) {
    printf("%d is aligned to %d\n", x, x & ~(x - 1));
  }
  while (1) ;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
