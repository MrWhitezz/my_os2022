#include <common.h>
#include <defs.h>

static void printl(void *ptr) {
  printf("%p%p\n", (uintptr_t)ptr >> 32, ptr);
}

int *ucreate_(task_t *task, const char *name) {
  assert(task != NULL);
  kmt->spin_lock(&tlk);
  AddrSpace *as = &task->as;
  protect(&task->as);
  printl(as->area.start);

  task->stack   = (uint8_t *)pmm->alloc(STK_SZ);
  task->name    = name;
  task->stat    = T_CREAT;
  task->is_run  = false;
  
  assert(task->stack != NULL);
  Area tstack   = RANGE(task->stack, (void *)task->stack + STK_SZ);
  Context *c    = ucontext(&task->as, tstack, task->as.area.start);
  task->context = c;
  
  kmt->spin_unlock(&tlk);
  add_task(task);

  return 0;
}