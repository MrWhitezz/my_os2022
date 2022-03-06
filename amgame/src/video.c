#include <game.h>

static int w, h;

extern struct craft ct;

static void init() {
  AM_GPU_CONFIG_T info = {0};
  AM_TIMER_CONFIG_T tmc   = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  ioe_read(AM_TIMER_CONFIG, &tmc);
  assert(tmc.has_rtc == true);
  w = info.width;
  h = info.height;
}

void update_ct(){


}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  init();
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if (1) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0x114514); // white
      }
    }
  }
  draw_tile(ct.x, ct.y, ct.w, ct.h, ct.color);
}
