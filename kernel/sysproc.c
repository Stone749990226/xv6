#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
// lab2
#include "sysinfo.h"

uint64 sys_exit(void) {
  int n;
  if (argint(0, &n) < 0) return -1;
  exit(n);
  return 0;  // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p;
  if (argaddr(0, &p) < 0) return -1;
  return wait(p);
}

uint64 sys_sbrk(void) {
  int addr;
  int n;

  if (argint(0, &n) < 0) return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0) return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0) return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0) return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// lab2
// implements the new system call by remembering its argument in a new variable in the proc structure (see
// kernel/proc.h)

// argint(0, &mask) 用于从用户程序的堆栈中提取整数参数，这个参数表示要执行 sys_trace 系统调用的目标进程的进程号（PID）
// argint 的第一个参数 0 表示要提取的参数在用户程序堆栈中的索引位置。
uint64 sys_trace(void) {
  int mask;
  // 获取追踪的mask
  if (argint(0, &mask) < 0) return -1;
  // 将mask保存在本进程的proc中
  myproc()->trace_mask = mask;
  return 0;
}

uint64 sys_sysinfo(void) {
  uint64 p;
  // argaddr:Retrieve an argument as a pointer.
  if (argaddr(0, &p) < 0) {
    return -1;
  }
  struct sysinfo info;
  info.freemem = get_free_memory_amount();
  info.nproc = get_proc_num();
  // copyout 参数：进程页表，用户态目标地址，数据源地址，数据大小
  // 返回值：数据大小
  if (copyout(myproc()->pagetable, p, (char *)&info, sizeof(info)) < 0) return -1;
  return 0;
}