#include <cpu.h>
#include <defs.h>

struct cpu cpus[NCPU] = {};
// Return this CPU's cpu struct.
// Interrupts must be disabled.

struct cpu* mycpu(void) {
  int id = cpu_current();
  struct cpu *c = &cpus[id];
  return c;
}