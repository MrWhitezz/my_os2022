#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <regex.h>
#include <time.h>
#define CALL_SZ 1000
#define NAME_SZ 100

extern char **environ;

typedef struct call_t {
  char *name;
  float us;
} call_t;

call_t Calls[CALL_SZ];

unsigned long gettimeus() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void call_add(char *name, float us) {
  int i;
  for (i = 0; i < CALL_SZ; i++) {
    if (Calls[i].name != NULL) {
      if (strcmp(Calls[i].name, name) == 0) {
        Calls[i].us += us;
        return;
      }
    }
  }
  for (i = 0; i < CALL_SZ; i++) {
    if (Calls[i].name == NULL) {
      Calls[i].name = strdup(name);
      Calls[i].us = us;
      return;
    }
  }
  return;
  assert(0);
}

void call_sort() {
  int i, j;
  for (i = 0; i < CALL_SZ; i++) {
    for (j = i + 1; j < CALL_SZ; j++) {
      if (Calls[i].us < Calls[j].us) {
        call_t tmp = Calls[i];
        Calls[i] = Calls[j];
        Calls[j] = tmp;
      }
    }
  }
}

void call_print_top5() {
  call_sort();
  float tot = 0;
  for (int i = 0; i < CALL_SZ; i++) {
    if (Calls[i].name != NULL)
      tot += Calls[i].us;
  }
  int i;
  for (i = 0; i < 5; i++) {
    if (Calls[i].name == NULL) break;
    int ratio = (int)(Calls[i].us * 100 / tot);
    printf("%s (%d%%)\n", Calls[i].name, ratio);
  }
  printf("\n");
  for (int i = 0; i < 80; ++i) putchar('\0');
  fflush(stdout);
}

int get_name(char *name, char *line){
  int len = 0;
  while (len < strlen(line) && len < NAME_SZ && line[len] != '(') {
    name[len] = line[len];
    len++;
  }
  if (len == strlen(line) || len == NAME_SZ) 
    return -1;
  name[len] = '\0';
  return 0;
}

float get_us(char *line){
  int pos = strlen(line) - 1;
  while (pos >= 0 && line[pos] != '<') {
    pos--;
  }
  if (line[pos] != '<') return 0;
  char us[512]; 
  ++pos;
  int i = 0;
  while (pos < strlen(line) && i < 512 && line[pos] != '>') {
    us[i] = line[pos];
    pos++; i++;
  }
  if (line[pos] != '>' || i == 512) return 0;
  us[i] = '\0';
  return (float )atof(us);
}

char **strace_argv(int argc, char *argv[], int fd) {
  char **new_argv = malloc(sizeof(char *) * (argc + 10));
  new_argv[0] = strdup("strace");
  new_argv[1] = strdup("-T");
  new_argv[2] = strdup("-o");
  char strace_file[100];
  sprintf(strace_file, "/proc/self/fd/%d", fd);
  new_argv[3] = strdup(strace_file);
  int pos = 4;
  for (int i = 1; i < argc; ++i) {
    new_argv[pos] = strdup(argv[i]);
    pos++;
  }
  new_argv[pos++] = NULL;
  return new_argv;
}

int main(int argc, char *argv[]) {
  memset(Calls, 0, sizeof(Calls));
  int fildes[2];
  if (pipe(fildes) == -1) {
    perror("pipe");
    exit(1);
  }

  char *path = getenv("PATH");

  int pid = fork();
  if (pid == 0){
    char **exec_argv = strace_argv(argc, argv, fildes[1]);
    close(fildes[0]);
    close(1);
    close(2);
    char *token;
    char *path_copy = strdup(path);
    token = strtok(path_copy, ":");
    while (token != NULL){
      char *cmd = malloc(sizeof(char) * (strlen(token) + strlen("/strace") + 2));
      strcpy(cmd, token);
      strcat(cmd, "/strace");
      execve(cmd, exec_argv, environ);
      token = strtok(NULL, ":");
    } 
    // should not reach here
    perror("execve");
    assert(0);
  }


  if (pid == 0){
    // child
  } else {
    dup2(fildes[0], 0);
    close(fildes[1]);

    char name[NAME_SZ];
    float us;

    char *line = malloc(sizeof(char) * 4096);
    size_t len = 0;
    unsigned long now = gettimeus();
    while (getline(&line, &len, stdin) != -1) {
      if (get_name(name, line) == -1) continue;
      if ((us = get_us(line)) == 0)   continue;
      call_add(name, us);
      unsigned long new = gettimeus();
      if (new - now > 1000000) {
        call_print_top5();
        now = new;
      }
    }
    call_print_top5();
  }

}
