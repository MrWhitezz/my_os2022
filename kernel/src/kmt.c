#include <os.h>
#include <cpu.h>
#include <defs.h>


#ifdef TEST_LOCAL
extern sem_t empty, fill;
#endif

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

// static void push_off(void) {
//   int old = ienabled();
//   iset(false);
  
//   struct cpu *c = mycpu();
//   if(c->noff == 0)
//     c->intena = old;
//   c->noff += 1;
// }

// static void pop_off(void) {
//   struct cpu *c = mycpu();

//   if(ienabled())
//     panic("pop_off - interruptible");
//   if(c->noff < 1)
//     panic("pop_off");
//   c->noff -= 1;
//   if(c->noff == 0 && c->intena)
//     iset(true);
// }


// spin lock

static void spin_init(spinlock_t *lk, const char *name){
  lk->locked = 0;
  lk->name = name;
  lk->cpu = -1; // no cpu holding the lock
}

static void spin_lock(spinlock_t *lk){
  // push_off(); // disable interrupts to avoid deadlock.
  int i = ienabled();
  iset(false);
  struct cpu *c = mycpu();
  if (c->noff == 0) c->intena = i;

  if(holding(lk)) {
    debug("cpu %d spin_lock %s\n", cpu_current(), lk->name);
    panic("acquire(spin_lock)");
  }

  long long cnt = 0;
  int prt = 0;
  while(atomic_xchg(&lk->locked, 1)) {
    // debug("cpu %d spin_lock %s\n", cpu_current(), lk->name);
    cnt++;
    panic_on(cnt > 1000000000LL, "spin_lock deadlock");
    if (cnt > 100000000LL && prt == 0) {
      debug("spinlk %s is held by cpu %d\n", lk->name, lk->cpu);
      debug("this cpu is %d cannot acquire\n", cpu_current());
      prt = 1;
    }
  }
  __sync_synchronize();

  c->noff += 1;
  lk->cpu = cpu_current();
}

static void spin_unlock(spinlock_t *lk) {
  if(!holding(lk)) {
    debug("lk %s : %d is held by %d\n", lk->name, lk->locked, lk->cpu);
    debug("this cpu %d is trying to unlock\n", cpu_current());
    panic("release(spin_unlock)");
  }

  lk->cpu = -1;

  // Release the lock, and restore interrupts.
  atomic_xchg(&lk->locked, 0);
  __sync_synchronize();

  assert(ienabled() == false);

  struct cpu *c = mycpu();
  assert(c->noff > 0);
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    iset(true);
  // pop_off();
}

static void sem_init(sem_t *sem, const char *name, int value) {
  sem->value = value;
  sem->name = name;
  spin_init(&sem->lock, name);
  sem->wait_list = createQueue(NTSK); // 32 to be modified
}


static void sem_wait(sem_t *sem) {
  // for debug // struct cpu *c1 = mycpu();
  // int off1 = c1->noff;
  // if (off1) {
  //   debug("off1 : %d on cpu %d\n", off1, cpu_current());
  //   debug("ienabled : %d\n", ienabled());
  //   debug("tlk is held by %d\n", tlk.cpu);
  //   debug("sem fill is held by %d\n", fill.lock.cpu);
  //   debug("sem empty is held by %d\n", empty.lock.cpu);
  // }
  assert(!holding(&sem->lock));
  assert(!holding(&tlk));
  // assert(c1->noff == 0);
  // seems bug here
  // TRACE_ENTRY;
  int acquire = 0;

  spin_lock(&sem->lock);
  // assert(mycpu()->noff == 1);
  assert(ienabled() == false);
  sem->value--;
  if (sem->value < 0) {
    enqueue(sem->wait_list, tcurrent);
    spin_lock(&tlk);
    assert(tcurrent != NULL && tcurrent->stat == T_RUNNABLE);
    tcurrent->stat = T_BLOCKED;
    spin_unlock(&tlk);
  } else {
    acquire = 1;
  }

  // debug("%s try to acquire(%d) on cpu %d, with sem->val: %d\n", tcurrent->name, acquire, cpu_current(), sem->value);
  spin_unlock(&sem->lock);
  // iset(false);

  // // for debug
  // assert(!holding(&sem->lock));
  // assert(!holding(&tlk));

  // struct cpu *c = mycpu();
  // int off = c->noff;
  // if (off) {
  //   debug("off : %d on cpu %d\n", off, cpu_current());
  //   debug("ienabled : %d\n", ienabled());
  //   debug("tlk is held by %d\n", tlk.cpu);
  //   debug("sem fill is held by %d\n", fill.lock.cpu);
  //   debug("sem empty is held by %d\n", empty.lock.cpu);
  // }
  // assert(c->noff == 0);
  // iset(true);
  // //

  if (!acquire) { 
    yield(); 
  }

  // while (sem->value < 0) {
  //   assert(ienabled() == false);
  //   assert(sem->value < 0);
  //   spin_unlock(&sem->lock);
  //   yield();
  //   spin_lock(&sem->lock);
  // }
  // TRACE_EXIT;
}

static void sem_signal(sem_t *sem) {
  // TRACE_ENTRY;
  // how to get thread id ?
  assert(!holding(&sem->lock));
  assert(!holding(&tlk));

  spin_lock(&sem->lock);
  sem->value++;
  if (!isEmpty(sem->wait_list)) {
    task_t *t = dequeue(sem->wait_list);
    spin_lock(&tlk);
    assert(t != NULL && t->stat == T_BLOCKED);
    t->stat = T_RUNNABLE;
    spin_unlock(&tlk);
  }
  spin_unlock(&sem->lock);
  // TRACE_EXIT;
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  assert(task != NULL);

  task->stack   = (uint8_t *)pmm->alloc(STK_SZ);
  task->name    = name;
  task->entry   = entry;
  task->arg     = arg;
  task->stat    = T_CREAT;
  task->is_run  = false;

  // must be called after task->stack is set
  assert(task->stack != NULL);
  Area tstack   = RANGE(task->stack, (void *)task->stack + STK_SZ);
  Context *c    = kcontext(tstack, entry, arg);
  task->context = c;
  add_task(task);
  return 0;
}

static void teardown(task_t *task) {
  assert(task->stat == T_CREAT);
  pmm->free(task->stack);
  del_task(task);
}

static void kmt_init() {

}

MODULE_DEF(kmt) = {
 // TODO
 .init        = kmt_init,
 .create      = kmt_create,
 .teardown    = teardown,
 .spin_init   = spin_init,
 .spin_lock   = spin_lock,
 .spin_unlock = spin_unlock,
 .sem_init    = sem_init,
 .sem_wait    = sem_wait,
 .sem_signal  = sem_signal,
};
