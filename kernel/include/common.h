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

#define STK_SZ (1 << 8)
enum tstat { T_RUNNABLE, T_RUNNING, T_BLOCKED, T_CREAT, };

struct task {
  struct {
    // int id; // temprorarily useless
    // task_t *next; // in thread-os.c to schedule, useless here
    enum tstat stat;
    const char *name;
    void (*entry)(void *arg);
    void *arg;
    Context *context;
  };
  uint8_t *stack;
};

struct spinlock {
  int locked;       // Is the lock held?

  // For debugging:
  const char *name;        // Name of lock.
  int  cpu;   // The cpu holding the lock.
};

struct semaphore {
  // TODO
  int value;
  spinlock_t lock;
  queue_t *wait_list;
  // For debugging:
  const char *name;
};

extern task_t *tasks[NTSK];
extern int tid;
extern spinlock_t tlk;
extern task_t *currents[NCPU];

#define tcurrent currents[cpu_current()]

#endif