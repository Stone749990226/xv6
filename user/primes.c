#include "kernel/types.h"
#include "user.h"
// 使用筛法获取35以内的质数
// 思路：prime创建父子进程，在父进程中，将数不断读出来，第一个读到的数base一定是质数，然后删掉它的倍数（将不是倍数的写入管道）
// 在子进程中，再调用prime函数继续筛
void prime(int parent_pipe[2]) {
  int pid;
  // 基数
  int base;
  // 如果没有读到任何东西说明筛法结束
  if (read(parent_pipe[0], &base, sizeof(int)) == 0) {
    exit(0);
  }
  printf("prime %d\n", base);
  int p[2];
  pipe(p);
  if ((pid = fork()) == 0) {  // 子进程为读端
    close(p[1]);
    prime(p);
  } else {
    close(p[0]);
    int n;
    while (read(parent_pipe[0], &n, sizeof(int))) {
      if (n % base != 0) {
        write(p[1], &n, sizeof(int));
      }
    }
    close(p[1]);
  }
  wait(&pid);
  exit(0);
}
int main(int argc, char *argv[]) {
  // 创建第一个管道，只往管道中写入数据2-35，然后关闭写端，调用prime函数
  int first_pipe[2];
  pipe(first_pipe);
  int i;
  for (i = 2; i < 36; i++) {
    write(first_pipe[1], &i, sizeof(int));
  }
  close(first_pipe[1]);
  prime(first_pipe);
  exit(0);
}