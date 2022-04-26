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


static void spin_init(spinlock_t *lk, const char *name){
  lk->locked = 0;
  lk->name = name;
  lk->cpu = -1; // no cpu holding the lock
}

static void spin_lock(spinlock_t *lk){
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire(spin_lock)");

  while(atomic_xchg(&lk->locked, 1))
    ;

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = cpu_current();
}

// Release the lock.
static void spin_unlock(spinlock_t *lk) {
  if(!holding(lk))
    panic("release");

  lk->cpu = -1;

  // Release the lock, and restore interrupts.
  atomic_xchg(&lk->locked, 0);

  pop_off();
}

MODULE_DEF(kmt) = {
 // TODO
 .spin_init = spin_init,
 .spin_lock = spin_lock,
 .spin_unlock = spin_unlock,
};
