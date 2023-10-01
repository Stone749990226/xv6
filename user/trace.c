#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
// 它期望从命令行接收两个或更多参数，第一个参数是追踪掩码（mask），用于按位指定要追踪的系统调用。第二个参数及之后的参数是要执行的命令和其参数。
int main(int argc, char *argv[]) {
  int i;
  char *nargv[MAXARG];

  if (argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')) {
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }

  for (i = 2; i < argc && i < MAXARG; i++) {
    nargv[i - 2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
