#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

void splash();
void print_key();

struct craft
{
  int x, y;
};

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

