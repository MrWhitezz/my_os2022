#include <common.h>
#include <cpu.h>
#include <defs.h>
#include "initcode.inc"
// every irq_handler will be called
// the first line of each function ensures 
// the event is matched when actually executing

static void pgmap(task_t *t, void *va, void *pa){
	t->va[t->np] = va;
	t->pa[t->np] = pa;
	t->np++;
	assert(t->np < NPG);
	map(&t->as, va, pa, MMAP_READ | MMAP_WRITE);
	debug("%s: va: %p%p, pa: %p%p\n", __func__, (uintptr_t)va >> 32, va, (uintptr_t)pa >> 32, pa);
}

static Context *pagefault(Event ev, Context *ctx) {
  if (ev.event != EVENT_PAGEFAULT) return NULL;
  assert(ev.event == EVENT_PAGEFAULT);
	// to be attention, I think no lock is needed here
	assert(!ienabled());
	AddrSpace *as = &(tcurrent->as);
	void *va = (void *)ROUNDDOWN(ev.ref, as->pgsize);
	void *pa = pmm->alloc(as->pgsize);
	assert(va >= as->area.start && va < as->area.end);

	if (va == as->area.start) {
		memcpy(pa, _init, _init_len);
	}
	pgmap(tcurrent, va, pa);
  return NULL;
}

static Context *syscall(Event ev, Context *ctx) {
  if (ev.event != EVENT_SYSCALL) return NULL;
	assert(mycpu()->noff == 0);
	assert(!ienabled());
	iset(true);
	int ret = 0;
	switch (ctx->GPRx) {

	}
  
	// ctx->GPRx = return value;
	iset(false);
	assert(tcurrent->context == ctx);
	ctx->GPRx = ret;
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