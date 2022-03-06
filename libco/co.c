#include "co.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define STACK_SIZE 1024 * 16
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
  const char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uintptr_t      sp;
  uint8_t        stack[STACK_SIZE]__attribute__((aligned(16))); // 协程的堆栈
};

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
// #if __x86_64__
//     "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
//       : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#if __x86_64__
    "movq %0, %%rsp;"
      : : "b"((uintptr_t)sp): "memory"


// #else
//     "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
//       : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#else
    "movl %0, %%esp"
      : : "b"((uintptr_t)sp - 8): "memory"
#endif
  );
}

static inline void stack_store(uintptr_t sp){
  asm volatile (
#if __x86_64__
    "movq %%rsp, (%0);"
      : : "r"((uintptr_t)sp): "memory"
#else
    "movl %%esp, (%0);"
      : : "r"((uintptr_t)sp): "memory"
#endif
  );
}

static inline void stack_change(void *sp) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp;"
      : : "b"((uintptr_t)sp): "memory"
#else
    "movl %0, %%esp"
      : : "b"((uintptr_t)sp - 8): "memory"
#endif
  );
}

struct co co_main = {};
struct co *current = &co_main;
struct co *POOL[MAXCO];


struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *c1 = malloc(sizeof(struct co));
  c1->name   = name;
  c1->func   = func;
  c1->arg    = arg;
  c1->status = CO_NEW;
  for (int i = 0; i < MAXCO; ++i){
    if (POOL[i] == NULL){
      POOL[i] = c1;
      break;
    }
  }
  return c1;
}

void co_wait(struct co *co) {
  debug("111\n");
  co->waiter = current;
  current->status = CO_WAITING;
  while (co->status != CO_DEAD){
    co_yield();
  }
  debug("\n %s finished\n", co->name);
  debug("current: %s\n", current->name);
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
  struct co *this_co = current;
  int val = setjmp(current->context);
  if (val == 0) {
    for (int i = 0; i < MAXCO; ++i){
      if (POOL[i] != NULL){
        if (POOL[i]->status == CO_RUNNING){
          current = POOL[i];
          longjmp(current->context, 1);
        }
        else if (POOL[i]->status == CO_NEW){
          // stack_store((uintptr_t)&current->sp);
          current = POOL[i];
          current->status = CO_RUNNING;
          stack_change(&current->stack[STACK_SIZE - 16 * sizeof(uintptr_t)]);
          ((current->func)(current->arg));

          current->status = CO_DEAD;
          current = this_co;
          
        }
      }
    }

  } else {
    // ?
  }
  debug("yield fi\n");
}