#include <common.h>

static Context *pagefault(Event ev, Context *ctx) {
  if (ev.event != EVENT_PAGEFAULT) return NULL;
  assert(ev.event == EVENT_PAGEFAULT);
  panic("pagefault not implemented");
  return NULL;
}

static Context *syscall(Event ev, Context *ctx) {
  if (ev.event != EVENT_SYSCALL) return NULL;
  assert(ev.event == EVENT_SYSCALL);
  panic("syscall not implemented");
  return NULL;
}

void irq_init() {
  os->on_irq(100, EVENT_PAGEFAULT, pagefault);
  os->on_irq(200, EVENT_SYSCALL, syscall);
}