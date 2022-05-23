#include <common.h>
#include <cpu.h>
#include <defs.h>
#include <devices.h>

spinlock_t tlk;

// tsks are distributed in currents, tsleeps, qtsks and sems without repeatness
// tlk protects currents, tsleeps, qtsks
queue_t *qtsks = NULL; 
task_t *currents[NCPU] = {}; 
task_t *tsleeps[NCPU] = {};
task_t *idles[NCPU] = {}; // need no lks, only visible by current cpu

handler_t trap_handlers[NHANDLER] = {};
int nhandler = 0;

void add_task(task_t *task) {
  kmt->spin_lock(&tlk);
  assert(!isFull(qtsks));
  enqueue(qtsks, task);
  kmt->spin_unlock(&tlk);
}

task_t *os_tsk_alloc() { 
  task_t *ret = (task_t *)pmm->alloc(sizeof(task_t)); 
  assert(ret != NULL);
  return ret;
}

#ifdef TEST_LOCAL
sem_t empty, fill;
#define P kmt->sem_wait
#define V kmt->sem_signal

void producer(void *arg) { while (1) { P(&empty); putch('('); V(&fill);  } }
void consumer(void *arg) { while (1) { P(&fill);  putch(')'); V(&empty); } }
void waste_time(void *arg) { while (1) { yield(); } }

spinlock_t sumlk;
int sum = 0;
void get_sum(void *arg) {
  for (int i = 0; i < 100000; i++) {
    kmt->spin_lock(&sumlk);
    sum ++;
    kmt->spin_unlock(&sumlk);
  }
  while (1) 
    debug("sum: %d, cpu: %d\n", sum, cpu_current());
}

void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  sprintf(ps, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}
#endif


static void os_init() {
  // single processor

  pmm->init();
  kmt->init();
  kmt->spin_init(&tlk, "tasks");
  uproc->init();
  // dev->init();
  irq_init(); // irq_handlers

#ifdef TEST_LOCAL
  // kmt->sem_init(&empty, "empty", 2);  // 缓冲区大小为 5
  // kmt->sem_init(&fill,  "fill",  0);
  // for (int i = 0; i < 20; i++) // 4 个生产者
  // {
  //   char *name = (char *)pmm->alloc(16);
  //   sprintf(name, "producer-%d", i);
  //   kmt->create(os_tsk_alloc(), name, producer, NULL);
  // }
  // for (int i = 0; i < 20; i++) // 5 个消费者
  // {
  //   char *name = (char *)pmm->alloc(16);
  //   sprintf(name, "consumer-%d", i);
  //   kmt->create(os_tsk_alloc(), name, consumer, NULL);
  // }
  for (int i = 0; i < 20; i++) // 10 个空转
  {
    char *name = (char *)pmm->alloc(16);
    sprintf(name, "waste-%d", i);
    kmt->create(os_tsk_alloc(), name, waste_time, NULL);
  }


  // kmt->create(os_tsk_alloc(), "tty_reader", tty_reader, "tty1");
  // kmt->create(os_tsk_alloc(), "tty_reader", tty_reader, "tty2");

#endif
  // the first user process
  char *name = (char *)pmm->alloc(16);
  sprintf(name, "user_proc");
  ucreate_(os_tsk_alloc(), name, NULL);
}


static void os_run() {
  // multi processor
  iset(true);
  while (1) {
    yield();
  }
}


static Context *kmt_sched(Event ev, Context *context) {
  assert(ienabled() == false); // because lock is held
  assert(!isEmpty(qtsks));
  Context *next = NULL;

  int ntry = 0;
  while (!isEmpty(qtsks)) {
    // switch to idle if no task is ready
    if (ntry > qtsks->size) {
      tcurrent = NULL;
      next = idles[cpu_current()]->context;
      break;
    }

    task_t *t = dequeue(qtsks);
    assert(t != NULL);
    assert(t->is_run == false 
    && (t->stat == T_CREAT || t->stat == T_RUNNABLE || t->stat == T_BLOCKED));
    if (t->stat == T_BLOCKED){
      enqueue(qtsks, t);
      ntry++;
      continue;
    }
    else {
      t->stat = T_RUNNABLE;
      t->is_run = true;
      tcurrent = t;
      next = tcurrent->context; 
      break;
    }
  }
  assert(next != NULL);
  return next;
}

static void sleep2queue(task_t *tslp, task_t *new) {
  assert(holding(&tlk));
  assert(tsleeps[cpu_current()] == tslp);
  if (tslp != NULL) {
    assert(tslp != new);
    assert(tslp->is_run == false && 
    (tslp->stat == T_BLOCKED || tslp->stat == T_SLEEPRUN || tslp->stat == T_RUNNABLE || tslp->stat == T_ZOMBIE));
    // tslp->stat can be T_BLOCKED(after P), T_SLEEPRUN(after sched), T_RUNNABLE(after V), T_ZOMBIE(after SYS_exit)
    if (tslp->stat == T_SLEEPRUN) {
      tslp->stat = T_RUNNABLE;
    }
    if (tslp-> stat != T_ZOMBIE && tslp != tcurrent) {
      // ensure that zombie tsk, interrupted nested tsk will not be added to queue
      enqueue(qtsks, tslp);
    }
    tsleeps[cpu_current()] = new;
  }
}

static Context *os_trap(Event ev, Context *context) {
  kmt->spin_lock(&tlk);
  // add sleep task to queue
  task_t *tslp = tsleeps[cpu_current()];
  if (tcurrent != NULL && tcurrent->is_run == false) {
    panic_on(tcurrent != tslp, "interrupted task is not tsleep");
    panic_on(!(ev.event == EVENT_IRQ_TIMER || ev.event == EVENT_YIELD),
      "interrupted task is not tsleep");
  }
  sleep2queue(tslp, NULL);

  // save current task and label it as sleep
  if (tcurrent != NULL) {
    tcurrent->is_run = false;
    tcurrent->context = context; 
    if (tcurrent->stat == T_RUNNABLE) {
      tcurrent->stat = T_SLEEPRUN;
    }
    else {
      assert(tcurrent->stat == T_BLOCKED); // blocked task is controlled by semaphore
    }
    tsleeps[cpu_current()] = tcurrent;
  }
  else {
    // save idle task
    int cpu = cpu_current();
    idles[cpu]->context = context;
  }

  kmt->spin_unlock(&tlk);
  assert(ienabled() == false);
  // ensure when enter irq_handler, tlk is unlocked
  for (int i = 0; i < nhandler; i++) {
    Context *ctx = (*trap_handlers[i])(ev, context);
    assert(ctx == NULL);
  }

  kmt->spin_lock(&tlk);
  task_t *t = tcurrent;
  task_t *tslp2 = tsleeps[cpu_current()];
	if (t != NULL && tslp2 != t) {
    sleep2queue(tslp2, t);
    assert(t->stat == T_RUNNABLE);
		t->is_run = false;
    t->stat = T_SLEEPRUN;
    t->context = context; // important!!!
	}
  Context *c = kmt_sched(ev, context);
  kmt->spin_unlock(&tlk);
  return c;
}




static void os_irq(int seq, int event, handler_t handler) {
  // handler should be added in order of seq
  debug("add handler %d with tot %d\n", seq, nhandler);
  assert(nhandler < NHANDLER);
  trap_handlers[nhandler++] = handler;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_irq,
};
