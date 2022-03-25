#include <common.h>
#include <thread.h>
#include <stdatomic.h>

static void entry(int tid) { pmm->alloc(128); }
static void goodbye()      { printf("End.\n"); }
int atomic_xchg(int *addr, int newval) {
  return atomic_exchange((int *)addr, newval);
}
Area heap = {};
int main() {
  pmm->init();
  for (int i = 0; i < 4; i++)
    create(entry);
  join(goodbye);
}