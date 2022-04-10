#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

extern char **environ;

typedef struct call_t {
  char *name;
  float us;
} call_t;

call_t Calls[200];

int get_name(char *name, char *line){
  int len = 0;
  while (len < strlen(line) && line[len] != '(') {
    name[len] = line[len];
    len++;
  }
  if (len == strlen(line))
    return -1;
  assert(len < 64);
  name[len] = '\0';
  return 0;
}

float get_us(char *line){
  int pos = strlen(line) - 1;
  while (pos >= 0 && line[pos] != '<') {
    pos--;
  }
  char us[64]; 
  if (line[pos] != '<') return 0;
  ++pos;
  int i = 0;
  while (pos < strlen(line) && line[pos] != '>') {
    us[i] = line[pos];
    pos++; i++;
  }
  us[i] = '\0';
  return (float )atof(us);
}

void print_argv(char *argv[]){
  for (int i = 0; argv[i] != NULL; ++i){
    printf("%s ", argv[i]);
  }
  printf("\n");
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
  int fildes[2];
  if (pipe(fildes) == -1) {
    perror("pipe");
    exit(1);
  }

  argv[0] = "strace";
  // char *exec_argv[] = { "strace", "ls", NULL, };
  // char *exec_envp[] = { "PATH=/bin", NULL, };
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
    char name[64];
    float us;

    char *line = malloc(sizeof(char) * 100);
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
      // printf("%s", line);
      char *s = line;

      if (get_name(name, line) == -1) 
        continue;
      if ((us = get_us(line)) == 0) 
        continue;
      call_t call = {name, us};
      printf("%s: %f\n", name, us);
    }
  }


  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
