#include <common.h>


task_t *tasks[NTSK];
int tid = 0;
spinlock_t tlk;

task_t *currents[NCPU]; // this need no lks

void add_task(task_t *task) {
  kmt->spin_lock(&tlk);
  while (tasks[tid] != NULL) {
    tid = (tid + 1) % NTSK;
  }
  assert(tasks[tid] == NULL);
  tasks[tid] = task; 
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

void producer(void *arg) { while (1) { P(&empty); putch('('); V(&fill);  } }
void consumer(void *arg) { while (1) { P(&fill);  putch(')'); V(&empty); } }

task_t *task_alloc() { return (task_t *)pmm->alloc(sizeof(task_t)); }
#endif

static void os_init() {
  // single processor
  printf("start os_init\n");

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
  printf("os_init() done.\n");
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
  debug("sched begin on cpu %d\n", cpu_current());
  kmt->spin_lock(&tlk);
  assert(ienabled() == false);
  task_t *t = NULL;
  do {
    while (tasks[tid] == NULL) {
      tid = (tid + 1) % NTSK;
    }
    // tcurrent = tasks[tid];
    t = tasks[tid];
  } while (!(t->stat == T_CREAT || t->stat == T_RUNNABLE));
  debug("out of sched loop on cpu %d\n", cpu_current());
  if (t->stat == T_CREAT) { t->stat = T_RUNNABLE; }
  debug("[sched] %s -> %s on cpu %d\n", tcurrent->name, t->name, cpu_current());
  tcurrent = t;
  Context *next = tcurrent->context;
  kmt->spin_unlock(&tlk);
  return next;
}

static Context *os_trap(Event ev, Context *context) {
  // ATTENTION: you should consider concurrency here.
  if (tcurrent != NULL) {
    kmt->spin_lock(&tlk);
    tcurrent->context = context; 
    kmt->spin_unlock(&tlk);
  }

  return kmt_sched(ev, context);
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
};
