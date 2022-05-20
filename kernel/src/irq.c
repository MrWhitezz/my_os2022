#include <common.h>

static Context *pagefault(Event ev, Context *ctx) {
  if (ev.event != EVENT_PAGEFAULT) return NULL;
  assert(ev.event == EVENT_PAGEFAULT);
  panic("pagefault not implemented");
  return NULL;
}

void irq_init() {
  os->on_irq(100, EVENT_PAGEFAULT, pagefault);
}