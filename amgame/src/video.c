#include <game.h>

#define BGC 0x114514
static int w, h;

extern struct craft ct;
static struct craft ct_old = {};

AM_TIMER_UPTIME_T tm = {};

static void init() {
  AM_GPU_CONFIG_T   info = {0};
  AM_TIMER_CONFIG_T tmc   = {0};   
  ioe_read(AM_GPU_CONFIG,   &info);
  ioe_read(AM_TIMER_CONFIG, &tmc);
  ioe_read(AM_TIMER_UPTIME, &tm);
  assert(tmc.has_rtc == true);
  w = info.width;
  h = info.height;
  ct_old = ct;
}

void update_ct(){
  uint32_t old_ms = tm.us / 1000; 
  ioe_read(AM_TIMER_UPTIME, &tm);
  uint32_t new_ms = tm.us / 1000; 
  uint32_t dis = (new_ms - old_ms) / 1000 * 10;
  printf("dis = %d\n", dis);
  switch (ct.direction){
    case UP    : ct.y = (ct.y + dis) % h;
    case DOWN  : ct.y = (ct.y - dis) % h;
    case RIGHT : ct.x = (ct.x + dis) % w;
    case LEFT  : ct.x = (ct.x - dis) % w;
  }
  
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
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, BGC); // white
      }
    }
  }
}

void splash_ct(){
  draw_tile(ct_old.x, ct_old.y, ct_old.w, ct_old.h, BGC);
  
  draw_tile(ct.x, ct.y, ct.w, ct.h, ct.color);

  ct_old = ct;
}