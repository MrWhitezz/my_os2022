#include <common.h>
#define STK_SZ (1 << 10)

struct task {
  int32_t id;
  const char *name;
  void (*entry)(void *arg);
  void *arg;
  // TODO
  uint8_t stack[STK_SZ];
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

  // For debugging:
  const char *name;
};

