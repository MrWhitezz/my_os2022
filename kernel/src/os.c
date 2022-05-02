#include <common.h>
#include <cpu.h>
#include <defs.h>



task_t *tasks[NTSK] = {};
int tid = 0;
spinlock_t tlk;

task_t *currents[NCPU] = {}; // this need no lks ??

void add_task(task_t *task) {
  kmt->spin_lock(&tlk);
  int oldtid = tid;
  while (tasks[tid] != NULL) {
    tid = (tid + 1) % NTSK;
    assert(tid != oldtid); // loop forever
  }
  assert(tasks[tid] == NULL);
  tasks[tid] = task; 
  tid = (tid + 1) % NTSK;
  kmt->spin_unlock(&tlk);
}

void del_task(task_t *task) {
  panic("del_task not implemented");
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
void waste_time(void *arg) { while (1) { yield(); } }

spinlock_t slk;
int sum = 0;
void get_sum(void *arg) {
  for (int i = 0; i < 100000; i++) {
    kmt->spin_lock(&slk);
    sum ++;
    kmt->spin_unlock(&slk);
  }
  while (1) 
    debug("sum: %d, cpu: %d\n", sum, cpu_current());
}

task_t *task_alloc() { 
  task_t *ret = (task_t *)pmm->alloc(sizeof(task_t)); 
  assert(ret != NULL);
  return ret;
}

// static void print_tasks() {
//   // should be called with tlk locked
//   for (int i = 0; i < NTSK; i++) {
//     if (tasks[i] != NULL) {
//       debug("%s: is_run: %d, stat: %d\n", tasks[i]->name, tasks[i]->is_run, tasks[i]->stat);
//     }
//   }
// }
#endif

static void os_init() {
  // single processor

  pmm->init();
  kmt->init();
  kmt->spin_init(&tlk, "tasks");


#ifdef TEST_LOCAL
  kmt->sem_init(&empty, "empty", 1);  // 缓冲区大小为 5
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
  // for (int i = 0; i < 50; i++) // 10 个空转
  // {
  //   char *name = (char *)pmm->alloc(16);
  //   sprintf(name, "waste-%d", i);
  //   kmt->create(task_alloc(), name, waste_time, NULL);
  // }

  // kmt->spin_init(&slk, "sum");
  // for (int i = 0; i < 100; i++) {
  //   char *name = (char *)pmm->alloc(16);
  //   sprintf(name, "get_sum-%d", i);
  //   kmt->create(task_alloc(), name, get_sum, NULL);
  // }

#endif
}


static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  iset(true);
  while (1) {
    yield();
  }
}

// static Context *input_notify(Event ev, Context *context) {
//   // kmt->sem_signal(&sem_kbdirq); // 在IO设备中断到来时，执行V操作唤醒一个线程
//   return NULL;
// }

static Context *kmt_sched(Event ev, Context *context) {
  // TRACE_ENTRY;
  // debug("sched begin on cpu %d\n", cpu_current());
  // kmt->spin_lock(&tlk);
  // debug("get lock on cpu %d\n", cpu_current());
  assert(ienabled() == false); // because lock is held
  task_t *t = NULL;
  int tid_inc = 0;
  do {
    while (tasks[tid] == NULL) {
      tid = (tid + 1) % NTSK;
      tid_inc++;
    }
    t = tasks[tid];
    tid = (tid + 1) % NTSK;
    tid_inc++;
    if (tid_inc > NTSK) {
      // print_tasks();  
      panic("tid_inc > NTSK");
    }
    assert(t != NULL);
  } while (!((t->stat == T_CREAT || t->stat == T_RUNNABLE) && t->is_run == false));
  // debug("out of sched loop on cpu %d\n", cpu_current());
  // if (t->stat == T_CREAT) { t->stat = T_RUNNABLE; }
  assert(t->is_run == false);
  t->stat = T_RUNNABLE;
  t->is_run = true;
  // debug("[sched] %s -> %s on cpu %d\n", tcurrent->name, t->name, cpu_current());
  tcurrent = t;
  // debug("sched to %s on cpu %d\n", t->name, cpu_current());
  Context *next = tcurrent->context;
  // kmt->spin_unlock(&tlk);
  // TRACE_EXIT;
  return next;
}

static Context *os_trap(Event ev, Context *context) {
  // ATTENTION: you should consider concurrency here.
  kmt->spin_lock(&tlk);
  // struct cpu *cc = mycpu();
  // int off = cc->noff;
  // if (off != 1) {
  //   debug("off = %d\n", off);
  //   assert(0);
  // }

  if (tcurrent != NULL) {
    // kmt->spin_lock(&tlk);
    tcurrent->context = context; 
    assert(tcurrent->is_run == true);
    tcurrent->is_run = false;
    // kmt->spin_unlock(&tlk);
  }

  if (ev.event == EVENT_YIELD) {

  }

  Context *c = kmt_sched(ev, context);
  kmt->spin_unlock(&tlk);
  return c;
}



static void os_irq(int seq, int event, handler_t handler) {
  assert(0);
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_irq,
};
