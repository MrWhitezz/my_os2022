#include <common.h>
#include <defs.h>

int *ucreate_(task_t *task, const char *name) {
  assert(task != NULL);
  protect(&task->as);

  task->stack   = (uint8_t *)pmm->alloc(STK_SZ);
  task->name    = name;
  task->stat    = T_CREAT;
  task->is_run  = false;
  
  assert(task->stack != NULL);
  Area tstack   = RANGE(task->stack, (void *)task->stack + STK_SZ);
  Context *c    = ucontext(&task->as, tstack, task->as.area.start);
  task->context = c;
  
  add_task(task);

  return 0;
}