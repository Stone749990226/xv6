#include "kernel/types.h"
#include "user.h"
#include "kernel/param.h"
// Shell会确保默认情况下，当一个程序启动时，文件描述符0连接到console的输入，文件描述符1连接到了console的输出
// 因此使用read(0,buf,1)即可从标准输入流中读取数据了
int main(int argc, char *argv[]) {
  // full_argv表示原参数+标准输入流拼接起来的参数
  char *full_argv[MAXARG];
  // instruct表示即将执行的指令
  char *instruct = argv[1];
  int i;
  for (i = 1; i < argc; i++) {
    full_argv[i - 1] = argv[i];
  }

  while (1) {
    int m = 0;
    // 从标准输入流中得到的外加参数将被保存在parameter字符数组里
    char parameter[500];
    while (read(0, &parameter[m], 1) && parameter[m] != '\n') {
      m++;
    }
    if (m == 0) {
      break;
    }
    parameter[m] = 0;
    // printf("\n%s\n\n", parameter);
    m = 0;
    full_argv[argc - 1] = parameter;
    // 子进程执行指令，父进程等待子进程
    if (fork() == 0) {
      exec(instruct, full_argv);
      exit(0);
    } else {
      wait(0);
    }
  }
  exit(0);
}