#include <os.h>

// Check whether this cpu is holding the lock.
// Interrupts must be off.
// static int holding(spinlock_t *lk) {
//   int r;
//   r = (lk->locked && lk->cpu == mycpu());
//   return r;
// }

static void spin_init(spinlock_t *lk, const char *name){
  lk->locked = 0;
  lk->name = name;
  lk->cpu = -1; // no cpu holding the lock
}

// void spin_lock(spinlock_t *lk){

   
// }

MODULE_DEF(kmt) = {
 // TODO
 .spin_init = spin_init,
};
