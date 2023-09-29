#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[]) {
  int pipe_to_child[2];
  int pipe_to_father[2];
  pipe(pipe_to_child);         // 父进程传给子进程
  pipe(pipe_to_father);        // 子进程传给父进程
  if (fork() == 0) {           // 如果是子进程
    close(pipe_to_child[1]);   // 关闭写端
    close(pipe_to_father[0]);  // 关闭读端
    char buffer[4];
    if (read(pipe_to_child[0], buffer, 4) > 0) {
      printf("%d: received ping\n", getpid());
      close(pipe_to_child[0]);
    }
    if (write(pipe_to_father[1], "pong", 4) != 4) {
      printf("write error!\n");
      exit(-1);
    }
    close(pipe_to_father[1]);  // 关闭写端
  } else {                     // 如果是父进程
    close(pipe_to_child[0]);   // 关闭读端
    close(pipe_to_father[1]);  // 关闭写端
    if (write(pipe_to_child[1], "ping", 4) != 4) {
      printf("write error!\n");
      exit(-1);
    }
    close(pipe_to_child[1]);  // 关闭写端
    char buffer[4];
    if (read(pipe_to_father[0], buffer, 4) > 0) {
      printf("%d: received pong\n", getpid());
      close(pipe_to_father[0]);
    }
  }
  exit(0);
}