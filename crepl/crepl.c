#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#ifdef __X86_64__
char arch[] = "-m64";
#else
char arch[] = "-m32";
#endif


static bool is_func(char *str) {
  while (*str && *str == ' ') str++;
  if (!str[0] || str[0] != 'i') return false; 
  if (!str[1] || str[1] != 'n') return false; 
  if (!str[2] || str[2] != 't') return false; 
  if (!str[3] || str[3] != ' ') return false; 
  return true;
}

int main(int argc, char *argv[]) {
  int dir = chdir("/tmp");
  if (dir != 0) {
    printf("[-] chdir failed\n");
    return 1;
  }

  char filename[] = "fuckXXXXXX";
  int fd = mkstemp(filename);
  if (fd == -1) {
    printf("[-] mkstemp failed\n");
    return 1;
  }
  else{
    dprintf(fd, "hello\n");
    printf("%s\n", filename);
  }

  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    printf("Got %zu chars.\n", strlen(line)); // ??
    if (is_func(line)) {
      printf("It's a function.\n");
    } else {
      printf("It's not a function.\n");
    }
  }
}
