#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

size_t nextPower_2(size_t x);

#ifdef LOCAL_DEBUG
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug(...)
#endif

