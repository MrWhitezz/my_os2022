#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

void print_argv(char *argv[]){
  for (int i = 0; argv[i] != NULL; ++i){
    printf("%s\n", argv[i]);
  }
}

char **argvdup(char *argv[]){
  int sz = 0;
  while (argv[sz] != NULL) sz++;
  char **new_argv = malloc(sizeof(char *) * (sz + 1));
  for (int i = 0; argv[i] != NULL; ++i){
    new_argv[i] = strdup(argv[i]);
  }
  new_argv[sz] = NULL;
  return new_argv;
}

int main(int argc, char *argv[]) {
  // char *exec_argv[] = { "strace", "ls", NULL, };
  argv[0] = "strace";
  // print_argv(argv);
  // print_argv(environ);

  // char *exec_argv[] = {"ls", "ls", NULL, };
  // char *exec_envp[] = { "PATH=/bin", NULL, };
  char **exec_argv = argv;
  char *path = getenv("PATH");

  char *exec_envp[] = { NULL, NULL, };
  printf("PATH: %s\n", getenv("PATH"));
  int pid = fork();
  if (pid == 0){
    printf("PATH: %s\n", getenv("PATH"));
    char *token;
    char *path_copy = strdup(path);
    token = strtok(path_copy, ":");
    printf("before loop\n");
    while (token != NULL){
      printf("PATH: %s\n", getenv("PATH"));
      char *cmd = malloc(sizeof(char) * (strlen(token) + strlen("/strace") + 2));
      strcpy(cmd, token);
      strcat(cmd, "/strace");
      print_argv(exec_envp);
      int ret = execve(cmd, exec_argv, exec_envp);
      token = strtok(NULL, ":");
    } 
  }

  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
