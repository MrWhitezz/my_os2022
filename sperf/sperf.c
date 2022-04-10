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

int main(int argc, char *argv[]) {
  // char *exec_argv[] = { "strace", "ls", NULL, };
  argv[0] = "strace";
  // print_argv(argv);
  // print_argv(environ);

  // char *exec_argv[] = {"ls", "ls", NULL, };
  // char *exec_envp[] = { "PATH=/bin", NULL, };
  char **exec_argv = argv;
  char **exec_envp = environ;

  char *path = getenv("PATH");
  int pid = fork();
  if (pid == 0){
    char *token;
    token = strtok(path, ":");
    while (token != NULL){
      char *cmd = malloc(sizeof(char) * (strlen(token) + strlen("/strace") + 2));
      strcpy(cmd, token);
      strcat(cmd, "/strace");
      execve(cmd, exec_argv, exec_envp);
      token = strtok(NULL, ":");
    } 
  }

  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
