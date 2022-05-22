#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <dsa.h>

size_t nextPower_2(size_t x);

// parameters

#define NCPU 8
#define NTSK 128
#define NHANDLER 256

// debug

#ifdef LOCAL_DEBUG
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

#ifdef TRACE_F
  #define TRACE_ENTRY debug("[trace] %s:entry\n", __func__)
  #define TRACE_EXIT  debug("[trace] %s:exit\n", __func__)
#else
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT  ((void)0)
#endif

// os related structs and variables

#define STK_SZ (1 << 12)
enum tstat { T_RUNNABLE, T_BLOCKED, T_CREAT, T_ZOMBIE, T_SLEEPRUN };

struct task {
  // struct {
    int id; // temprorarily useless
    // task_t *next; // in thread-os.c to schedule, useless here
    enum tstat stat;
    bool is_run;
    const char *name;
    void (*entry)(void *arg);
    void *arg;
    AddrSpace as;
    Context *context;
  // };
  uint8_t *stack;
};

struct spinlock {
  int locked;       // Is the lock held?

  // For debugging:
  const char *name;        // Name of lock.
  int  cpu;   // The cpu holding the lock.
};

struct semaphore {
  int value;
  spinlock_t lock;
  queue_t *wait_list;
  // For debugging:
  const char *name;
};

// extern task_t *tasks[NTSK];
// extern int tid;
extern queue_t *qtsks;
extern spinlock_t tlk;
extern task_t *currents[NCPU];
extern task_t *idles[NCPU];

#define tcurrent currents[cpu_current()]

#endif