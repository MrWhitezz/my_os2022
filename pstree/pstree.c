#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

struct process {
  pid_t pid;
  char name[64];
  pid_t ppid;
};

bool show_pid = false;
bool num_sort = false;
bool version  = false;
int parse_args(int argc, char *argv[]);
int get_pnum_load(int i, struct process pro[]);
enum{PNUM, LOAD};

void rm_paren(char *dst);

int main(int argc, char *argv[]) {

  parse_args(argc, argv);

  int pnum = get_pnum_load(PNUM, NULL);

  struct process *pro = malloc(pnum * sizeof(struct process));

  get_pnum_load(LOAD, pro);


  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);
  
  return 0;
}

int get_pnum_load(int i, struct process pro[]){
  int num = 0;
  DIR *d = opendir("/proc");
  struct dirent *dir;
  if (d){
    while ((dir = readdir(d)) != NULL){
      if (atoi(dir->d_name) != 0){
        printf("find %s\n", dir->d_name);
        if (i == LOAD) {
          char stat_name[64] = "/proc/";
          strcat(stat_name, dir->d_name);
          strcat(stat_name, "/stat");
          printf("Open %s\n", stat_name); 

          FILE *fp = fopen(stat_name, "r");
          if (fp){
            char s = 'S';
            fscanf(fp, "%d %s %c %d", &pro[num].pid, pro[num].name, &s, &pro[num].ppid);
            fclose(fp);
            rm_paren(pro[num].name);
            printf("pid: %d, comm: %s, ppid: %d\n", pro[num].pid, pro[num].name, pro[num].ppid);
          }
          else {assert(0);}

        }
        num ++;
      }
    }
  }
  return num;
}

void rm_paren(char *dst){
  int len = strlen(dst);
  char tmp[64] = "";
  strncpy(tmp, dst + 1, len - 2);
  strncpy(dst, tmp, len - 2);
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