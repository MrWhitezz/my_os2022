#include <common.h>


task_t *tasks[NTSK] = {};
int tid = 0;
spinlock_t tlk;

task_t *currents[NCPU] = {}; // this need no lks ??

void add_task(task_t *task) {
  kmt->spin_lock(&tlk);
  while (tasks[tid] != NULL) {
    tid = (tid + 1) % NTSK;
  }
  assert(tasks[tid] == NULL);
  tasks[tid] = task; 
  tid = (tid + 1) % NTSK;
  kmt->spin_unlock(&tlk);
}

void del_task(task_t *task) {
  kmt->spin_lock(&tlk);
  while (tasks[tid] != task) {
    tid = (tid + 1) % NTSK;
  }
  assert(tasks[tid] == task);
  tasks[tid] = NULL; 
  kmt->spin_unlock(&tlk);
}

#ifdef TEST_LOCAL
sem_t empty, fill;
#define P kmt->sem_wait
#define V kmt->sem_signal

void producer(void *arg) { while (1) { yield(); P(&empty); putch('('); V(&fill);  yield();} }
void consumer(void *arg) { while (1) { yield(); P(&fill);  putch(')'); V(&empty); yield();} }

task_t *task_alloc() { 
  task_t *ret = (task_t *)pmm->alloc(sizeof(task_t)); 
  // debug("task_alloc: %p with sz 0x%x\n", ret, sizeof(task_t));
  assert(ret != NULL);
  return ret;
}
#endif

static void os_init() {
  // single processor

  pmm->init();
  kmt->init();
  kmt->spin_init(&tlk, "tasks");


#ifdef TEST_LOCAL
  kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
  kmt->sem_init(&fill,  "fill",  0);
  for (int i = 0; i < 4; i++) // 4 个生产者
  {
    char *name = (char *)pmm->alloc(16);
    sprintf(name, "producer-%d", i);
    kmt->create(task_alloc(), name, producer, NULL);
  }
  for (int i = 0; i < 5; i++) // 5 个消费者
  {
    char *name = (char *)pmm->alloc(16);
    sprintf(name, "consumer-%d", i);
    kmt->create(task_alloc(), name, consumer, NULL);
  }
#endif
}


static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  iset(true);
  while (1) ;
}

// static Context *input_notify(Event ev, Context *context) {
//   // kmt->sem_signal(&sem_kbdirq); // 在IO设备中断到来时，执行V操作唤醒一个线程
//   return NULL;
// }

static Context *kmt_sched(Event ev, Context *context) {
  TRACE_ENTRY;
  // debug("sched begin on cpu %d\n", cpu_current());
  kmt->spin_lock(&tlk);
  // debug("get lock on cpu %d\n", cpu_current());
  assert(ienabled() == false);
  task_t *t = NULL;
  do {
    while (tasks[tid] == NULL) {
      tid = (tid + 1) % NTSK;
    }
    t = tasks[tid];
    tid = (tid + 1) % NTSK;
    assert(t != NULL);
  } while (!(t->stat == T_CREAT || t->stat == T_RUNNABLE));
  // debug("out of sched loop on cpu %d\n", cpu_current());
  if (t->stat == T_CREAT) { t->stat = T_RUNNABLE; }
  // debug("[sched] %s -> %s on cpu %d\n", tcurrent->name, t->name, cpu_current());
  tcurrent = t;
  // debug("sched to %s on cpu %d\n", t->name, cpu_current());
  Context *next = tcurrent->context;
  kmt->spin_unlock(&tlk);
  TRACE_EXIT;
  return next;
}

static Context *os_trap(Event ev, Context *context) {
  // ATTENTION: you should consider concurrency here.
  if (tcurrent != NULL) {
    kmt->spin_lock(&tlk);
    tcurrent->context = context; 
    kmt->spin_unlock(&tlk);
  }

  if (ev.event == EVENT_YIELD) {

  }

  return kmt_sched(ev, context);
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
};
