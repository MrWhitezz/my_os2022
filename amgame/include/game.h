#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define SIDE 16
void splash();
void print_key();
void init_ct();


struct craft
{
  int x, y, w, h;
  int direction;
  uint32_t color;
};
enum{UP, DOWN, LEFT, RIGHT};


static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

