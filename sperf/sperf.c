#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_argv(int argc, char *argv[]){
  for (int i = 0; i < argc; ++i){
    printf("%s\n", argv[i]);
  }
}

int main(int argc, char *argv[]) {
  // char *exec_argv[] = { "strace", "ls", NULL, };
  argv[0] = "strace";
  // char *exec_argv[] = {"ls", "ls", NULL, };
  char *exec_argv[] = argv;
  char *exec_envp[] = { "PATH=/bin", NULL, };

  execve("strace",          exec_argv, exec_envp);
  execve("/bin/strace",     exec_argv, exec_envp);
  execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
