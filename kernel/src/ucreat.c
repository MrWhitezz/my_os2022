#include <common.h>

int *ucreate_(task_t *task, const char *name) {
  assert(task != NULL);
  protect(&task->as);

  task->stack  = (uint8_t *)pmm->alloc(STK_SZ);
  task->name   = name;
  task->entry  = 0;
  task->stat   = T_CREAT;
  task->is_run = false;
  assert(0);

  return 0;
}