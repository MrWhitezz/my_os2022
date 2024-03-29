#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dlfcn.h>

#ifdef __x86_64__
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

int nexp = 0;

bool is_valid(char *line){
  char filetmp[] = "tmpXXXXXX";
  mkstemp(filetmp);
  FILE *fp = fopen(filetmp, "w");
  fprintf(fp, "%s\n", line);
  fclose(fp);
  int pid = fork();
  if (pid == 0) {
    execlp("gcc", "gcc", "-fPIC", "-shared", arch, "-o", "/dev/null", "-x", "c", filetmp, NULL);
  }
  int wstatus = 0;
  waitpid(pid, &wstatus, 0);
  if (WEXITSTATUS(wstatus)){
    assert(WIFEXITED(wstatus));
    return false;
  }
  return true;
}

bool func_handler(char *line){
  if (!is_valid(line)) return false;
  FILE *fp = fopen(filename, "a");
  fprintf(fp, "%s\n", line);
  fclose(fp);
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
    execvp("gcc", argv);
  }
  wait(NULL);
  return true;
}

void expr_handler(char *line){
  char expr[1024];
  char name[128];
  sprintf(name, "__expr_wrapper_%d", nexp++);
  sprintf(expr, "int %s(){return %s;}", name, line);
  if (func_handler(expr)){
    printf("ready to call %s\n", name);
    void *handle = dlopen("/tmp/libcrepl.so", RTLD_LAZY);
    if (!handle) {
      printf("%s\n", dlerror());
      return;
    }
    int (*func)();
    func = dlsym(handle, name);
    if (!func) {
      printf("%s\n", dlerror());
      return;
    }
    printf("%d\n", func());
    dlclose(handle);
  }
}

int main(int argc, char *argv[]) {
  int dir = chdir("/tmp");
  if (dir != 0) {
    printf("[-] chdir failed\n");
    return 1;
  }

  int fd = mkstemp(filename);
  assert(fd >= 0);

  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    if (is_func(line)) {
      func_handler(line);
    } else {
      expr_handler(line);
    }
  }
}
