#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

extern char **environ;

void print_argv(char *argv[]){
  for (int i = 0; argv[i] != NULL; ++i){
    printf("%s\n", argv[i]);
  }
}

// char **argvdup(char *argv[]){
//   int sz = 0;
//   while (argv[sz] != NULL) sz++;
//   char **new_argv = malloc(sizeof(char *) * (sz + 1));
//   for (int i = 0; argv[i] != NULL; ++i){
//     new_argv[i] = strdup(argv[i]);
//   }
//   new_argv[sz] = NULL;
//   return new_argv;
// }

char **strace_argv(int argc, char *argv[]) {
  char **new_argv = malloc(sizeof(char *) * (argc + 10));
  new_argv[0] = strdup("strace");
  new_argv[1] = strdup("-T");
  new_argv[2] = strdup("-o");
  new_argv[3] = strdup("strace.log");
  int pos = 4;
  for (int i = 1; i < argc; ++i) {
    new_argv[pos] = strdup(argv[i]);
    pos++;
  }
  new_argv[pos++] = strdup(">");
  new_argv[pos++] = strdup("/dev/null");
  new_argv[pos++] = strdup("2>");
  new_argv[pos++] = strdup("/dev/null");
  new_argv[pos++] = NULL;
  return new_argv;
}

int main(int argc, char *argv[]) {
  argv[0] = "strace";
  // char *exec_argv[] = { "strace", "ls", NULL, };
  // char *exec_envp[] = { "PATH=/bin", NULL, };
  char **exec_argv = strace_argv(argc, argv);
  char *path = getenv("PATH");

  print_argv(exec_argv);

  int pid = fork();
  if (pid == 0){
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




  // execve("strace",          exec_argv, exec_envp);
  // execve("/bin/strace",     exec_argv, exec_envp);
  // execve("/usr/bin/strace", exec_argv, exec_envp);
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
