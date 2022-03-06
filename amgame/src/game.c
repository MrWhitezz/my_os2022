#include <game.h>

struct craft ct;

extern int w, h;

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}

void init_ct(){
  ct.x = w / 2;
  ct.y = h / 2;
  ct.w = SIDE;
  ct.h = SIDE;
  ct.color = 0x121119;
}