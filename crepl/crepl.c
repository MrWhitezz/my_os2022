#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#ifdef __X86_64__
char arch[] = "-m64";
#else
char arch[] = "-m32";
#endif
// char CFLAGS[] = "-fPIC -shared";

char filename[] = "fuckXXXXXX";

static bool is_func(char *str) {
  while (*str && *str == ' ') str++;
  if (!str[0] || str[0] != 'i') return false; 
  if (!str[1] || str[1] != 'n') return false; 
  if (!str[2] || str[2] != 't') return false; 
  if (!str[3] || str[3] != ' ') return false; 
  return true;
}

bool is_valid(char *line){
  char filetmp[] = "tmpXXXXXX";
  mkstemp(filetmp);
  FILE *fp = fopen(filetmp, "w");
  fprintf(fp, "%s\n", line);
  int wstatus = 0;
  int pid = fork();
  if (pid == 0) {
    execlp("/usr/bin/gcc", "gcc", "-fPIC", "-shared", arch, "-o", "/dev/null", "-x", "c", filetmp, NULL);
  }
  if (wait(&wstatus) == -1) {
    fclose(fp);
    assert(0);
  }
  fclose(fp);
  printf("wstatus = %d\n", wstatus);
  if (WEXITSTATUS(wstatus)){
    printf("gcc failed\n");
    int es = WEXITSTATUS(wstatus);
    printf("%d\n", es);
    return false;
  }
  printf("gcc success\n");
  return true;
}

void func_handler(char *line){
  if (!is_valid(line)) return;
  FILE *fp = fopen(filename, "a");
  fprintf(fp, "%s\n", line);
  char *argv[] = {
    "gcc",
    "-fPIC",
    "-shared",
    arch,
    "-o",
    "libcrepl.so",
    "-x",
    "c",
    filename,
    NULL,
  };
  int pid = fork();
  if (pid == 0) {
    execvp("/usr/bin/gcc", argv);
  }
  wait(NULL);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int dir = chdir("/tmp");
  if (dir != 0) {
    printf("[-] chdir failed\n");
    return 1;
  }

  int fd = mkstemp(filename);
  assert(fd >= 0);
  printf("[+] filename: %s\n", filename);

  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    if (is_func(line)) {
      printf("It's a function.\n");
      func_handler(line);
    } else {
      printf("It's not a function.\n");
    }
  }
}
