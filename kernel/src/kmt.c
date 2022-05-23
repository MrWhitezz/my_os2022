#include <os.h>
#include <cpu.h>
#include <defs.h>


#ifdef TEST_LOCAL
extern sem_t empty, fill;
#endif

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

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
    panic("acquire(spin_lock)");
  }

  long long cnt = 0;
  while(atomic_xchg(&lk->locked, 1)) {
    cnt++;
    panic_on(cnt > 1000000000LL, "spin_lock deadlock");
  }
  __sync_synchronize();

  c->noff += 1;
  lk->cpu = cpu_current();
}

static void spin_unlock(spinlock_t *lk) {
  if(!holding(lk)) {
    panic("release(spin_unlock)");
  }

  lk->cpu = -1;

  atomic_xchg(&lk->locked, 0);
  __sync_synchronize();

  assert(ienabled() == false);

  struct cpu *c = mycpu();
  assert(c->noff > 0);
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    iset(true);
}

static void sem_init(sem_t *sem, const char *name, int value) {
  sem->value = value;
  sem->name = name;
  spin_init(&sem->lock, name);
  sem->wait_list = createQueue(NTSK); // 32 to be modified
}


static void sem_wait(sem_t *sem) {
  assert(!holding(&sem->lock));
  assert(!holding(&tlk));

  int acquire = 0;

  spin_lock(&sem->lock);
  assert(ienabled() == false);
  sem->value--;
  if (sem->value < 0) {
    enqueue(sem->wait_list, tcurrent);
    spin_lock(&tlk);
    assert(tcurrent != NULL && tcurrent->stat == T_RUNNABLE);
    tcurrent->stat = T_BLOCKED; // should be out of qtsks
    spin_unlock(&tlk);
  } else {
    acquire = 1;
  }

  spin_unlock(&sem->lock);

  if (!acquire) { 
    yield(); 
  }
}

static void sem_signal(sem_t *sem) {
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
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  assert(task != NULL);

  task->stack   = (uint8_t *)pmm->alloc(STK_SZ);
  task->name    = name;
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
  panic("teardown not implemented");
  assert(task->stat == T_CREAT);
  pmm->free(task->stack);
  task->stat = T_ZOMBIE;
}


static void kmt_init() {
  qtsks = createQueue(NTSK);
  for (int i = 0; i < cpu_count(); ++i) {
    idles[i] = os_tsk_alloc();
  }
  debug("kmt_init: qtsks init, idles created\n");
}


MODULE_DEF(kmt) = {
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
