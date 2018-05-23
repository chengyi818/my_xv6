#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  /*
    延迟分配内存,因此在sbrk()时不实际分配物理内存,仅增加进程可用的虚拟内存空间.
  */

  /* addr = myproc()->sz; */
  /* if(growproc(n) < 0) */
  /*   return -1; */

  // new
  struct proc *curproc = myproc();
  addr = curproc->sz;

  if((uint)addr+n > KERNBASE)
    return -1;

  if(n < 0)
    return addr;

  curproc->sz += n;

  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// date系统调用实际实现
int
sys_date(void) {
  struct rtcdate* r;
  // 从用户空间栈顶获取参数
  if(argptr(0, (void*)&r, sizeof(*r)) < 0)
    return -1;

  cmostime(r);

  return 0;
}

// alarm系统调用实际实现
int
sys_alarm(void)
{
  int ticks;
  void (*handler)();

  if(argint(0, &ticks) < 0)
    return -1;
  if(argptr(1, (char**)&handler, 1) < 0)
    return -1;
  myproc()->alarmticks = ticks;
  myproc()->alarmhandler = handler;
  //cprintf("sys_alarm called\n");
  return 0;
}
