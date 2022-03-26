#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

unsigned int nextPower_2(unsigned int x);

#ifdef LOCAL_DEBUG
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

