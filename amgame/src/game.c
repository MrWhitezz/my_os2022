#include <game.h>

struct craft ct;

// Operating system is a C program!
int main(const char *args) {
  ioe_init();
  init_ct();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
    splash_ct();
  }
  return 0;
}

void init_ct(){
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  int w = info.width;
  int h = info.height;
  ct.x = w / 2;
  ct.y = h / 2;
  ct.w = SIDE;
  ct.h = SIDE;
  ct.color = 0xffffff;
}