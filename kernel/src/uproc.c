#include <os.h>
#include <syscall.h>

#include "initcode.inc"

static void *pgalloc(int n) {
  return pmm->alloc(n);
}

static void uproc_init() {
	vme_init(pgalloc, pmm->free);	
  printf("uproc_init\n");
  // need to init vme
}

static int u_kputc(task_t *task, char ch) {
  putch(ch);
  return 0;
}

MODULE_DEF(uproc) = {
  .init = uproc_init,
  .kputc = u_kputc,
};