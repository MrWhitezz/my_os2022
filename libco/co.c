#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define STACK_SIZE 64
#define MAXCO      128 + 5

#ifdef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
};

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
// #if __x86_64__
//     "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
//       : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#if __x86_64__
    "movq %0, %%rsp;"
      : : "b"((uintptr_t)sp): "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
  );
}

struct co co_main = {};
struct co *current = &co_main;
struct co *POOL[MAXCO];


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *c1 = malloc(sizeof(struct co));
  c1->name   = name;
  assert(0);
  c1->func   = func;
  c1->arg    = arg;
  c1->status = CO_NEW;
  assert(0);
  for (int i = 0; i < MAXCO; ++i){
    if (POOL[i] == NULL){
      POOL[i] = c1;
      break;
    }
  }
  return c1;
}

void co_wait(struct co *co) {
  assert(0);
  co->waiter = current;
  current->status = CO_WAITING;
  while (co->status != CO_DEAD){
    co_yield();
  }
  for (int i = 0; i < MAXCO; ++i){
    if (POOL[i] == co){
      POOL[i] = NULL;
      break;
    }
  }
  free(co);
  // unsure
  current->status = CO_RUNNING;
}

void co_yield() {
  assert(0);
  int val = setjmp(current->context);
  if (val == 0) {
    for (int i = 0; i < MAXCO; ++i){
      if (POOL[i] != NULL){
        if (POOL[i]->status == CO_RUNNING){
          current = POOL[i];
          longjmp(current->context, 1);
        }
        else if (POOL[i]->status == CO_NEW){
          current = POOL[i];
          current->status = CO_RUNNING;
          
          assert(0); 
          stack_switch_call(&current->stack[STACK_SIZE - 1 - sizeof(uintptr_t)], current->func, (uintptr_t)current->arg);
          assert(0); 
          ((current->func)(current->arg));
          current->status = CO_DEAD;
        }
      }
    }

  } else {
    // ?
  }
}