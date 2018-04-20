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
  int signal_number;

  if(argint(0, &pid) < 0) {
    return -1;
  }

  if(argint(1, &signal_number) < 0) {
    return -1;
  }
  
  return kill(pid, signal_number);
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
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
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

//This will update the process signal mask, returns old mask.
uint
sys_sigprocmask(void) {
  uint signal_mask;
  argint(0, (int *)&signal_mask);
  return sigprocmask(signal_mask);
}

//This will update the process signal handler.
uint
sys_signal(void) {
  sighandler_t handler;
  int signal_number;
  if(argint(0, &signal_number) < 0){
    return -1;
  }
  if(argptr(1, (void*) &handler, sizeof(handler)) < 0) {
    return -1;
  }
  return (uint)signal(signal_number, handler);
}

void
sys_sigret(void)
{
  sigret();
}
