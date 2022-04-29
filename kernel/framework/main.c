#include <klib.h>
#include <am.h>
#include <klib-macros.h>
#include <common.h>

int main() {
  ioe_init();
  cte_init(os->trap);
  os->init();
  mpe_init(os->run);
  return 1;
}
