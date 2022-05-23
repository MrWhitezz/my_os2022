#include <os.h>
#include <syscall.h>

// #include "initcode.inc"

static void *pgalloc(int n) {
  return pmm->alloc(n);
}

static void uproc_init() {
	vme_init(pgalloc, pmm->free);	
  printf("uproc_init\n");
  // need to init vme
}

static int u_kputc(task_t *t, char ch) {
  putch(ch);
  return 0;
}

static int u_fork(task_t *t) {
  panic("fork not implemented");
  return 0;
}

static int u_wait (task_t *t, int *status) {
  panic("wait not implemented");
  return 0;
}

static int u_exit(task_t *t, int status) {
  panic("exit not implemented");
  return 0;
}

static int u_kill(task_t *t, int pid) {
  panic("kill not implemented");
  return 0;
}

static void *u_mmap(task_t *t, void *addr, int length, int prot, int flags) {
  panic("mmap not implemented");
  return 0;
}

static int u_getpid(task_t *t) {
  panic("getpid not implemented");
  return 0;
}

static int u_sleep(task_t *t, int seconds) {
  panic("sleep not implemented");
  return 0;
}

static int64_t u_uptime(task_t *t) {
  panic("uptime not implemented");
  return 0;
}

MODULE_DEF(uproc) = {
  .init   = uproc_init,
  .kputc  = u_kputc,
  .fork   = u_fork,
  .wait   = u_wait,
  .exit   = u_exit,
  .kill   = u_kill,
  .mmap   = u_mmap,
  .getpid = u_getpid,
  .sleep  = u_sleep,
  .uptime = u_uptime,
};