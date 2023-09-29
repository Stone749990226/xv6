#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user.h"

void find(char *path, char *fn) {
  // 存储临时路径的数组buf和指针p
  char buf[512], *p;
  // 文件描述符（File Descriptor），它用于打开文件或目录并执行与该文件描述符相关的操作
  int fd;
  // 目录项（Directory Entry），用于存储目录中的每个文件或子目录的信息
  struct dirent de;
  // 文件或目录的元数据信息
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // buf存储临时路径
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';
  // 循环读取每个目录项
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    // 将目录项的名称补充到路径上
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (stat(buf, &st) < 0) {
      printf("ls: cannot stat %s\n", buf);
      continue;
    }
    // 如果目录项是文件，则直接比较文件名是否相等；如果是路径，则递归调用find函数
    switch (st.type) {
      case T_FILE:
        if (strcmp(fn, de.name) == 0) {
          printf("%s\n", buf);
        }
        break;
      case T_DIR:
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
          continue;
        } else {
          find(buf, fn);
        }
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("find needs two argument!\n");
    exit(-1);
  }
  char *path = argv[1];
  // 文件名file name
  char *fn = argv[2];
  find(path, fn);
  exit(0);
}