#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_argv(int argc, char *argv){
  for (int i = 0; i < argc; ++i){
    printf("%s\n", argv[i]);
  }
}

int main(int argc, char *argv[]) {
  // char *exec_argv[] = { "strace", "ls", NULL, };
  printf("argc = %d\n", argc);
  print_argv(argc, argv);
  argv[0] = "strace";
  print_argv(argc, argv);
  char *exec_argv[] = {"ls", "ls", NULL, };
  char *exec_envp[] = { "PATH=/bin", NULL, };
  
  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
