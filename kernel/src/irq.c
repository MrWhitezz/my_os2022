#include <common.h>

// every irq_handler will be called
// the first line of each function ensures 
// the event is matched when actually executing
static Context *pagefault(Event ev, Context *ctx) {
  if (ev.event != EVENT_PAGEFAULT) return NULL;
  assert(ev.event == EVENT_PAGEFAULT);
	printf("pf: %p by %p\n", ev.ref, ctx->rip);
  // panic("pagefault not implemented");
  return NULL;
}

static Context *syscall(Event ev, Context *ctx) {
  if (ev.event != EVENT_SYSCALL) return NULL;
  assert(ev.event == EVENT_SYSCALL);
	assert(tcurrent->context == ctx);
	// ctx->GPRx = return value;
  panic("syscall not implemented");
  return NULL;
}

static Context *irq_yield(Event ev, Context *ctx) {
	if (ev.event != EVENT_YIELD) return NULL;
  return NULL;
}

static Context *irq_error(Event ev, Context *ctx) {
	if (ev.event != EVENT_ERROR) return NULL;
	panic("error not implemented");
	return NULL;
}

void irq_init() {
  os->on_irq(100, EVENT_PAGEFAULT, pagefault);
  os->on_irq(200, EVENT_SYSCALL, syscall);
	os->on_irq(300, EVENT_YIELD, irq_yield);
	os->on_irq(400, EVENT_ERROR, irq_error);
}