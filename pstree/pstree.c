#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>

bool show_pid = false;
bool num_sort = false;
bool version  = false;
int parse_args(int argc, char *argv[]);
int get_pnum();
struct process {
  pid_t pid;
  char name[64];
  pid_t ppid;
};

int main(int argc, char *argv[]) {
  parse_args(argc, argv);

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  
  get_pnum();
  return 0;
}

int get_pnum(){
  int num = 0;
  DIR *d = opendir("/proc");
  struct dirent *dir;
  if (d){
    while ((dir = readdir(d)) != NULL){
      if (atoi(dir->d_name) != 0){
        num ++;
        printf("%s\n", dir->d_name);
      }
    }
  }
  return num;
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
      case 'p': show_pid = true; printf("PPP!\n"); break;
      case 'n': num_sort = true; printf("NNN!\n"); break;
      case 'V': version  = true; printf("VVV!\n"); break;
      case 1: printf("You give wrong opt!!!\n"); break;
    }
  }
  return 0;
}