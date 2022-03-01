#include "co.h"
#include <stdlib.h>

#ifdef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

struct co {
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  return NULL;
}

void co_wait(struct co *co) {
}

void co_yield() {
}


int main(){
  debug(dddd);
}