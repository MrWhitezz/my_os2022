#include <os.h>
#include <cpu.h>
#include <defs.h>


// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.
static void push_off(void) {
  int old = ienabled();

  iset(false);
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

static void pop_off(void) {
  struct cpu *c = mycpu();
  if(ienabled())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    iset(true);
}


// spin lock

static void spin_init(spinlock_t *lk, const char *name){
  lk->locked = 0;
  lk->name = name;
  lk->cpu = -1; // no cpu holding the lock
}

static void spin_lock(spinlock_t *lk){
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire(spin_lock)");

  while(atomic_xchg(&lk->locked, 1)) {
    yield();
  }

  lk->cpu = cpu_current();
}

static void spin_unlock(spinlock_t *lk) {
  if(!holding(lk))
    panic("release(spin_unlock)");

  lk->cpu = -1;

  // Release the lock, and restore interrupts.
  atomic_xchg(&lk->locked, 0);

  pop_off();
}

static void sem_init(sem_t *sem, const char *name, int value) {
  sem->value = value;
  sem->name = name;
  spin_init(&sem->lock, name);
  sem->wait_list = createQueue(32);
}


static void sem_wait(sem_t *sem) {
  // seems bug here
  int acquire = 0;
  spin_lock(&sem->lock);
  assert(ienabled() == false);
  sem->value--;
  if (sem->value < 0) {
    enqueue(sem->wait_list, tcurrent);
    tcurrent->stat = T_BLOCKED;
  } else {
    acquire = 1;
  }

  spin_unlock(&sem->lock);
  if (!acquire) { yield(); }

  // while (sem->value < 0) {
  //   assert(ienabled() == false);
  //   assert(sem->value < 0);
  //   spin_unlock(&sem->lock);
  //   yield();
  //   spin_lock(&sem->lock);
  // }
}

static void sem_signal(sem_t *sem) {
  // how to get thread id ?
  spin_lock(&sem->lock);
  sem->value++;
  if (!isEmpty(sem->wait_list)) {
    task_t *t = dequeue(sem->wait_list);
    t->stat = T_RUNNABLE;
  }
  spin_unlock(&sem->lock);
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {

  task->stack   = pmm->alloc(STK_SZ);
  task->name    = name;
  task->entry   = entry;
  task->arg     = arg;
  task->stat    = T_CREAT;

  // must be called after task->stack is set
  Area tstack   = RANGE(task->stack, (void *)task->stack + STK_SZ);
  Context *c    = kcontext(tstack, entry, arg);
  task->context = c;
  return 0;
}

static void teardown(task_t *task) {
  assert(task->stat == T_CREAT);
  pmm->free(task->stack);
}

MODULE_DEF(kmt) = {
 // TODO
 .create      = kmt_create,
 .teardown    = teardown,
 .spin_init   = spin_init,
 .spin_lock   = spin_lock,
 .spin_unlock = spin_unlock,
 .sem_init    = sem_init,
 .sem_wait    = sem_wait,
 .sem_signal  = sem_signal,
};
