#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <getopt.h>
bool show_pid = false;
bool num_sort = false;
int parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  parse_args(argc, argv);

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  return 0;
}

int parse_args(int argc, char *argv[]){
  const struct option table[] = {
    {"show-pids"    , no_argument, NULL, 'p'},
    {"numeric-sort" , no_argument, NULL, 'n'},
    {"version"      , no_argument, NULL, 'V'}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "-pnV", table, NULL)) != -1){
    switch (opt){
      case 1: printf("You give wrong opt!!!"); break;
    }
  }
  return 0;
}