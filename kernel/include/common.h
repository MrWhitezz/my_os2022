#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <os.h>

#ifndef __COMMON_H__
#define __COMMON_H__

size_t nextPower_2(size_t x);

// parameters

#define NCPU 8

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

#endif