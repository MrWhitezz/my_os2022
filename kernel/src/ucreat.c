#include <common.h>
#include <defs.h>


int *ucreate_(task_t *task, const char *name, void *entry) {
  assert(task != NULL);
  // I think no lock is needed
  protect(&task->as);

  task->stack   = (uint8_t *)pmm->alloc(STK_SZ);
  task->name    = name;
  task->stat    = T_CREAT;
  task->is_run  = false;
  task->id      = get_new_pid();
  
  if (entry == NULL)
    entry = task->as.area.start;
  assert(task->stack != NULL);
  Area tstack   = RANGE(task->stack, (void *)task->stack + STK_SZ);
  Context *c    = ucontext(&task->as, tstack, entry);
  task->context = c;
  
  add_task(task);

  return 0;
}