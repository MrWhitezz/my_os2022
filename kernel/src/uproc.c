#include <os.h>
#include <syscall.h>

// #include "initcode.inc"

static void *pgalloc(int n) {
  return pmm->alloc(n);
}

static void uproc_init() {
	vme_init(pgalloc, pmm->free);	
  printf("uproc_init: vme_init finished\n");
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
  t->stat = T_ZOMBIE;
  return status;
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
  return t->id;
}

static int u_sleep(task_t *t, int seconds) {
  uint64_t wakeup = io_read(AM_TIMER_UPTIME).us + seconds * 1000000L;
  while (io_read(AM_TIMER_UPTIME).us < wakeup) {
    // debug("%d us left\n", wakeup - io_read(AM_TIMER_UPTIME).us);
    yield();
  }
  // debug("wakeup\n");
  return 0;
}

static int64_t u_uptime(task_t *t) {
  uint64_t ms = io_read(AM_TIMER_UPTIME).us / 1000L;
  return ms;
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