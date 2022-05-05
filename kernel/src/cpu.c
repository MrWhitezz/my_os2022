#include <cpu.h>
#include <os.h>
#include <defs.h>

struct cpu cpus[NCPU] = {};


// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu* mycpu(void) {
  int id = cpu_current();
  struct cpu *c = &cpus[id];
  return c;
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int holding(spinlock_t *lk) {
  int r;
  r = (lk->locked && lk->cpu == cpu_current());
  // if (r) {
  //   assert(ienabled() == 0);
  //   debug("cpu %d holding %s\n", cpu_current(), lk->name);
  // }
  return r;
}