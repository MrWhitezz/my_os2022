#include <game.h>

extern struct craft ct;

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = {
  AM_KEYS(KEYNAME)
};

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
    if (event.keycode == AM_KEY_ESCAPE){
      halt(1);
    }
    switch (event.keycode){
      case AM_KEY_K     :
      case AM_KEY_UP    : ct.direction = UP;    break;
      case AM_KEY_J     :
      case AM_KEY_DOWN  : ct.direction = DOWN;  break;
      case AM_KEY_H     :
      case AM_KEY_LEFT  : ct.direction = LEFT;  break;
      case AM_KEY_L     :
      case AM_KEY_RIGHT : ct.direction = RIGHT; break;
    }
  }
}
