#include "co.h"
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifdef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

#define STACK_SIZE (1024 * (64 + 2))

#define CANARY_SZ  256
#define STK_OFF    (1024 + 16 * sizeof(uintptr_t))
#define MAXCO      (128 + 8)

#define MAGIC 0x99999999
#define BOTTOM (STACK_SIZE / sizeof(uint32_t) - 1)

// void canary_init(void *p) {
//   uint32_t *ptr = (uint32_t *)p;
//   for (int i = 0; i < CANARY_SZ; i++)
//     ptr[BOTTOM - i] = ptr[i] = MAGIC;
//     ;
// }

// void canary_check(void *p) {
//   uint32_t *ptr = (uint32_t *)p;
//   for (int i = 0; i < CANARY_SZ; i++) {
//     assert(ptr[BOTTOM - i] == MAGIC);
//     assert(ptr[i] == MAGIC);
//   }
// }

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
  uint8_t        stack[STACK_SIZE]__attribute__((aligned(16))); // 协程的堆栈
};

static inline void stack_change(void *sp) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp;"
      : : "r"((uintptr_t)sp): "memory"
#else
    "movl %0, %%esp"
      : : "r"((uintptr_t)sp): "memory"
#endif
  );
}

struct co co_main = {.name = "main", .status = CO_RUNNING, .waiter = NULL};
struct co *current = &co_main;
struct co *POOL[MAXCO] = {&co_main};
int    cert[MAXCO];
int    ct_sz = 0;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  srand(time(NULL));
  struct co *c1 = malloc(sizeof(struct co));
  c1->name   = name;
  c1->func   = func;
  c1->arg    = arg;
  c1->waiter = NULL;
  c1->status = CO_NEW;
  // canary_init(&c1->stack[0]);
  for (int i = 0; i < MAXCO; ++i){
    if (POOL[i] == NULL){
      POOL[i] = c1;
      break;
    }
  }
  return c1;
}

void co_wait(struct co *co) {
  debug("Begin wait %s at %p\n", co->name, co);
  debug("current: %s at %p\n", current->name, current);

  co->waiter = current;
  current->status = CO_WAITING;
  
  while (co->status != CO_DEAD){
    co_yield();
  }

  debug("%s finished and will be freed!\n", co->name);
  debug("current: %s\n", current->name);
  for (int i = 0; i < MAXCO; ++i){
    if (POOL[i] == co){
      POOL[i] = NULL;
      break;
    }
  }
  free(co);
  debug("success wait!\n");
  // unsure
  current->status = CO_RUNNING;
}

void co_yield() {
  // if (current != &co_main)
  //   canary_check(&current->stack[0]);

  int val = setjmp(current->context);
  if (val == 0) {
    ct_sz = 0;
    for (int i = 0; i < MAXCO; ++i){
      if (POOL[i] != NULL && (POOL[i]->status == CO_RUNNING || POOL[i]->status == CO_NEW)){
        cert[ct_sz++] = i;
        // if (ct_sz > 120) break;
        // debug("%d ", i);
      }
    }


    if (ct_sz > 0){
      int index = cert[rand() % ct_sz];
      // assert(POOL[index] != NULL);
      if (POOL[index]->status == CO_RUNNING){
          current = POOL[index];
          longjmp(current->context, 1);
      }
      else if (POOL[index]->status == CO_NEW){
        // debug("There's new co %s\n", POOL[i]->name);
        current = POOL[index];
        current->status = CO_RUNNING;
        // stack_switch_call(&current->stack[STACK_SIZE - STK_OFF], current->func, (uintptr_t)current->arg);
        stack_change(&current->stack[STACK_SIZE - STK_OFF]);
        ((current->func)(current->arg));

        current->status = CO_DEAD;
        debug("%s DEAD at %p ", current->name, current);
        if (current->waiter != NULL){
          debug("TRY_RUN_WAIT %p ", current->waiter);
          current->waiter->status = CO_RUNNING;
        }

        co_yield();
        assert(0);
      }  
    }
  }
    
  else {

  }
}