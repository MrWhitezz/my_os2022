#include <common.h>
#include <cpu.h>
#include <defs.h>

spinlock_t tlk;

task_t *currents[NCPU] = {}; // this need no lks ??
task_t *tsleeps[NCPU] = {};

void add_task(task_t *task) {
  kmt->spin_lock(&tlk);
  assert(!isFull(qtsks));
  enqueue(qtsks, task);
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
  kmt->sem_init(&empty, "empty", 2);  // 缓冲区大小为 5
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
  for (int i = 0; i < 10; i++) // 10 个空转
  {
    char *name = (char *)pmm->alloc(16);
    sprintf(name, "waste-%d", i);
    kmt->create(task_alloc(), name, waste_time, NULL);
  }

  // kmt->spin_init(&slk, "sum");
  // for (int i = 0; i < 100; i++) {
  //   char *name = (char *)pmm->alloc(16);
  //   sprintf(name, "get_sum-%d", i);
  //   kmt->create(task_alloc(), name, get_sum, NULL);
  // }

#endif
}


static void os_run() {
  iset(true);
  yield();
}

// static Context *input_notify(Event ev, Context *context) {
//   // kmt->sem_signal(&sem_kbdirq); // 在IO设备中断到来时，执行V操作唤醒一个线程
//   return NULL;
// }

static Context *kmt_sched(Event ev, Context *context) {
  assert(ienabled() == false); // because lock is held
  while (isEmpty(qtsks)) {
    assert(0);
  }
  
  task_t *t = dequeue(qtsks);
  assert(t != NULL);
  assert(t->is_run == false && (t->stat == T_CREAT || t->stat == T_RUNNABLE));
  
  t->stat = T_RUNNABLE;
  t->is_run = true;
  tcurrent = t;
  Context *next = tcurrent->context;
  return next;
}

static Context *os_trap(Event ev, Context *context) {
  // to be modifed, should add stat sleep
  kmt->spin_lock(&tlk);
  task_t *tslp = tsleeps[cpu_current()];
  if (tslp != NULL) {
    assert(tslp->is_run == false && (tslp->stat == T_BLOCKED || tslp->stat == T_SLEEPRUN));
    if (tslp->stat == T_SLEEPRUN) {
      tslp->stat = T_RUNNABLE;
      enqueue(qtsks, tslp);
    }
    else {
      assert(tslp->stat == T_BLOCKED);
      tslp->stat = T_WAKEBLK;
    }
    tsleeps[cpu_current()] = NULL;
  }

  if (tcurrent != NULL) {
    tcurrent->context = context; 
    assert(tcurrent->is_run == true);
    tcurrent->is_run = false;
    if (tcurrent->stat == T_RUNNABLE) {
      tcurrent->stat = T_SLEEPRUN;
    }
    else {
      assert(tcurrent->stat == T_BLOCKED); // blocked task is controlled by semaphore
    }
    tsleeps[cpu_current()] = tcurrent;
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
