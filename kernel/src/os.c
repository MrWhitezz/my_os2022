#include <common.h>


task_t *tasks[NTSK];
int tid = 0;
spinlock_t tlk;

task_t *currents[NCPU]; // this need no lks

// sem_t empty, fill;
// #define P kmt->sem_wait
// #define V kmt->sem_signal

// void producer(void *arg) { while (1) { P(&empty); putch('('); V(&fill);  } }
// void consumer(void *arg) { while (1) { P(&fill);  putch(')'); V(&empty); } }

static void os_init() {
  // single processor
  pmm->init();
  kmt->init();
  kmt->spin_init(&tlk, "tasks");

#ifdef TEST_LOCAL
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < 4; i++) // 4 个生产者
    kmt->create(task_alloc(), "producer", producer, NULL);
  for (int i = 0; i < 5; i++) // 5 个消费者
    kmt->create(task_alloc(), "consumer", consumer, NULL);
#endif
}


static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  iset(true);
  while (1) ;
}

static Context *os_trap(Event ev, Context *context) {
  // ATTENTION: you should consider concurrency here.
  if (tcurrent != NULL) {

  }
  return NULL;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
};
