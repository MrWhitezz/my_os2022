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
void print_pro(struct process pro[], int pnum, int id, int depth);
void pro_sort(struct process pro[], int pnum);

int main(int argc, char *argv[]) {

  parse_args(argc, argv);

  if (version) {
    printf("pstree (PSmisc) 23.4\n \
  Copyright (C) 1993-2020 Werner Almesberger and Craig Small\
  \n\
  PSmisc comes with ABSOLUTELY NO WARRANTY.\n\
  This is free software, and you are welcome to redistribute it under\n\
  the terms of the GNU General Public License.\n\
  For more information about these matters, see the files named COPYING.\n");
    return 0;
  }

  int pnum = get_pnum_load(PNUM, NULL);

  struct process *pro = malloc((pnum + 5) * sizeof(struct process));

  get_pnum_load(LOAD, pro);  

  if (num_sort)
    pro_sort(pro, pnum);

  print_pro(pro, pnum, 1, 0);
  
  return 0;
}

void put_tab(int n){
  for (int i = 0; i < 2 * n; ++i)
    putchar(' ');
}

void print_pro(struct process pro[], int pnum, int id, int depth){
  for (int i = 0; i < pnum; ++i) {
    if (pro[i].pid == id){
      put_tab(depth);
      printf("%s\n", pro[i].name);
      for (int j = 0; j < pnum; ++j){
        if (pro[j].ppid == id){
          print_pro(pro, pnum, pro[j].pid, depth + 1);
        }
      }
    }
  }
}

void pro_sort(struct process pro[], int pnum){
  for (int i = 0; i < pnum; ++i){
    int min_id = i;
    for (int j = i; j < pnum; ++j){
      min_id = (pro[j].pid < pro[min_id].pid) ? j : min_id;
    }
    struct process tmp = pro[i];
    pro[i] = pro[min_id];
    pro[min_id] = tmp;
  }
}

int get_pnum_load(int i, struct process pro[]){
  int num = 0;
  DIR *d = opendir("/proc");
  struct dirent *dir;
  if (d){
    while ((dir = readdir(d)) != NULL){
      if (atoi(dir->d_name) != 0){
        // printf("find %s\n", dir->d_name);
        if (i == LOAD) {
          char stat_name[64] = "/proc/";
          strcat(stat_name, dir->d_name);
          strcat(stat_name, "/stat");
          // printf("Open %s\n", stat_name); 

          FILE *fp = fopen(stat_name, "r");
          if (fp){
            char s = 'S';
            fscanf(fp, "%d %s %c %d", &pro[num].pid, pro[num].name, &s, &pro[num].ppid);
            fclose(fp);
            rm_paren(pro[num].name);
            if (show_pid){
              strcat(pro[num].name, "(");
              strcat(pro[num].name, dir->d_name);
              strcat(pro[num].name, ")");
            }
            printf("pid: %d,\tcomm: %s,\tppid: %d\n", pro[num].pid, pro[num].name, pro[num].ppid);
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
  strcpy(dst, tmp);
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
      case 'p': show_pid = true; break;
      case 'n': num_sort = true; break;
      case 'V': version  = true; break;
      case 1: printf("You give wrong opt!!!\n"); exit(0); break;
    }
  }
  return 0;
}