#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

extern char **environ;

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
    char tmp[100];
    while (fgets(tmp, 100, stdin) != NULL){
      printf("%s", tmp);
    }
  }




  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
